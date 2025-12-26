#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include <cmath>

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fVerbose(false),
  fVerboseMaxEvents(5)
{
    // Initialiser les noms des volumes d'eau pour identification rapide
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fWaterRingNames.insert(DetectorConstruction::GetWaterRingName(i) + "Log");
    }
}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // ═══════════════════════════════════════════════════════════════
    // RÉCUPÉRATION DES INFORMATIONS DE BASE
    // ═══════════════════════════════════════════════════════════════

    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // Vérifier que les volumes existent
    if (!preStepPoint->GetPhysicalVolume()) {
        return;
    }

    G4String preVolumeName = preStepPoint->GetPhysicalVolume()->GetName();
    
    G4String postVolumeName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postVolumeName = postStepPoint->GetPhysicalVolume()->GetName();
    }

    // Informations sur la trace
    G4Track* track = step->GetTrack();
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4double kineticEnergy = preStepPoint->GetKineticEnergy();

    // ID de l'événement pour le debug
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DANS LES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════

    G4LogicalVolume* volume = preStepPoint->GetTouchableHandle()
                                          ->GetVolume()
                                          ->GetLogicalVolume();
    G4String logicalVolumeName = volume->GetName();

    // Vérifier si on est dans un anneau d'eau
    if (fWaterRingNames.find(logicalVolumeName) != fWaterRingNames.end()) {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0.) {
            // Extraire l'index de l'anneau depuis le nom du volume
            // Format: "WaterRing_X" -> on extrait X
            G4int ringIndex = -1;
            
            // Chercher quel anneau
            for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
                G4String ringLogName = DetectorConstruction::GetWaterRingName(i) + "Log";
                if (logicalVolumeName == ringLogName) {
                    ringIndex = i;
                    break;
                }
            }
            
            if (ringIndex >= 0) {
                fEventAction->AddRingEnergy(ringIndex, edep);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4cout << "  WATER | Event " << eventID
                           << " | Ring " << ringIndex
                           << " | " << particleName
                           << " edep=" << edep/keV << " keV"
                           << G4endl;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION AU PLAN UPSTREAM
    // ═══════════════════════════════════════════════════════════════

    if (postVolumeName == "UpstreamDetector" && preVolumeName != "UpstreamDetector") {

        G4ThreeVector momentum = track->GetMomentumDirection();
        G4double pz = momentum.z();

        if (pz > 0) {
            if (parentID == 0 && particleName == "gamma") {
                fEventAction->RecordPrimaryUpstream(trackID, kineticEnergy);

                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4cout << "  UPSTREAM | Event " << eventID
                           << " | PRIMARY gamma trackID=" << trackID
                           << " E=" << kineticEnergy/keV << " keV"
                           << G4endl;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION AU PLAN DOWNSTREAM
    // ═══════════════════════════════════════════════════════════════

    if (postVolumeName == "DownstreamDetector" && preVolumeName != "DownstreamDetector") {

        G4ThreeVector momentum = track->GetMomentumDirection();
        G4double pz = momentum.z();

        if (pz > 0) {
            if (parentID == 0 && particleName == "gamma") {
                fEventAction->RecordPrimaryDownstream(trackID, kineticEnergy);

                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4cout << "  DOWNSTREAM | Event " << eventID
                           << " | PRIMARY gamma trackID=" << trackID
                           << " E=" << kineticEnergy/keV << " keV"
                           << G4endl;
                }

            } else {
                G4String processName = "Unknown";
                const G4VProcess* creatorProcess = track->GetCreatorProcess();
                if (creatorProcess) {
                    processName = creatorProcess->GetProcessName();
                }

                G4int pdgCode = track->GetDefinition()->GetPDGEncoding();

                fEventAction->RecordSecondaryDownstream(
                    trackID,
                    parentID,
                    pdgCode,
                    kineticEnergy,
                    processName
                );

                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4cout << "  DOWNSTREAM | Event " << eventID
                           << " | SECONDARY " << particleName
                           << " trackID=" << trackID
                           << " parentID=" << parentID
                           << " E=" << kineticEnergy/keV << " keV"
                           << " process=" << processName
                           << G4endl;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DEBUG DÉTAILLÉ (optionnel)
    // ═══════════════════════════════════════════════════════════════

    if (fVerbose && eventID < fVerboseMaxEvents) {
        G4ThreeVector prePos = preStepPoint->GetPosition();
        G4ThreeVector postPos = postStepPoint->GetPosition();

        if (parentID == 0 && particleName == "gamma") {
            G4cout << "  Step | trackID=" << trackID
                   << " " << preVolumeName << " → " << postVolumeName
                   << " z: " << prePos.z()/mm << " → " << postPos.z()/mm << " mm"
                   << " E=" << kineticEnergy/keV << " keV"
                   << G4endl;
        }
    }
}
