#include "SteppingAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Logger.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include <cmath>
#include <sstream>

SteppingAction::SteppingAction(EventAction* eventAction, RunAction* runAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fRunAction(runAction),
  fVerbose(true),           // ACTIVÉ pour vérification
  fVerboseMaxEvents(10)     // Afficher les 10 premiers événements
{
    // Initialiser les noms des volumes d'eau pour identification rapide
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fWaterRingNames.insert(DetectorConstruction::GetWaterRingName(i) + "Log");
    }
    
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  SteppingAction: Mode VERBOSE activé pour " << fVerboseMaxEvents << " événements     ║" << G4endl;
    G4cout << "║  Diagnostics -> output.log                                     ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
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
                    G4ThreeVector pos = preStepPoint->GetPosition();
                    G4double radius = std::sqrt(pos.x()*pos.x() + pos.y()*pos.y());
                    
                    std::stringstream ss;
                    ss << "WATER_DEPOSIT | Event " << eventID
                       << " | Ring " << ringIndex
                       << " | " << particleName
                       << " | E_kin=" << kineticEnergy/keV << " keV"
                       << " | edep=" << edep/keV << " keV"
                       << " | r=" << radius/mm << " mm"
                       << " | z=" << pos.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION (toujours actifs)
    // ═══════════════════════════════════════════════════════════════
    
    G4String postLogVolName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postLogVolName = postStepPoint->GetPhysicalVolume()->GetLogicalVolume()->GetName();
    }
    
    // Entrée dans le filtre (gamma primaire)
    if (postLogVolName == "FilterLog" && logicalVolumeName != "FilterLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementFilterEntry();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "FILTER_ENTRY | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Sortie du filtre (gamma primaire)
    if (logicalVolumeName == "FilterLog" && postLogVolName != "FilterLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementFilterExit();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                G4double postEnergy = postStepPoint->GetKineticEnergy();
                std::stringstream ss;
                ss << "FILTER_EXIT | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << postEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Entrée dans le container (gamma primaire)
    if ((postLogVolName == "ContainerWallLog" || postLogVolName == "ContainerTopLog") 
        && logicalVolumeName != "ContainerWallLog" && logicalVolumeName != "ContainerTopLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementContainerEntry();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "CONTAINER_ENTRY | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Entrée dans l'eau
    if (fWaterRingNames.find(postLogVolName) != fWaterRingNames.end() 
        && fWaterRingNames.find(logicalVolumeName) == fWaterRingNames.end()) {
        
        if (particleName == "gamma") {
            fRunAction->IncrementWaterEntry();
        }
        if (particleName == "e-") {
            fRunAction->IncrementElectronsInWater();
        }
        
        if (fVerbose && eventID < fVerboseMaxEvents) {
            G4ThreeVector pos = postStepPoint->GetPosition();
            G4double radius = std::sqrt(pos.x()*pos.x() + pos.y()*pos.y());
            std::stringstream ss;
            ss << "WATER_ENTRY | Event " << eventID
               << " | " << particleName
               << " | trackID=" << trackID
               << " | parentID=" << parentID
               << " | E=" << kineticEnergy/keV << " keV"
               << " | r=" << radius/mm << " mm"
               << " | z=" << pos.z()/mm << " mm"
               << " | " << postLogVolName;
            Logger::GetInstance()->LogLine(ss.str());
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLANS DE COMPTAGE CYLINDRIQUES (gammas uniquement, direction +z)
    // ═══════════════════════════════════════════════════════════════
    
    G4ThreeVector momentum = step->GetTrack()->GetMomentumDirection();
    G4double pz = momentum.z();
    
    // Plan pré-filtre (traversée dans le sens +z)
    if (postLogVolName == "PreFilterPlaneLog" && logicalVolumeName != "PreFilterPlaneLog") {
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPreFilterPlane();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_FILTER_PLANE | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Plan post-filtre (traversée dans le sens +z)
    if (postLogVolName == "PostFilterPlaneLog" && logicalVolumeName != "PostFilterPlaneLog") {
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPostFilterPlane();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "POST_FILTER_PLANE | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Plan pré-eau (traversée dans le sens +z)
    if (postLogVolName == "PreWaterPlaneLog" && logicalVolumeName != "PreWaterPlaneLog") {
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPreWaterPlane();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_WATER_PLANE | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Plan post-eau (traversée dans le sens +z)
    if (postLogVolName == "PostWaterPlaneLog" && logicalVolumeName != "PostWaterPlaneLog") {
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPostWaterPlane();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "POST_WATER_PLANE | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
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
                    std::stringstream ss;
                    ss << "UPSTREAM | Event " << eventID
                       << " | PRIMARY gamma trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV";
                    Logger::GetInstance()->LogLine(ss.str());
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
                    std::stringstream ss;
                    ss << "DOWNSTREAM | Event " << eventID
                       << " | PRIMARY gamma trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV";
                    Logger::GetInstance()->LogLine(ss.str());
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
                    std::stringstream ss;
                    ss << "DOWNSTREAM | Event " << eventID
                       << " | SECONDARY " << particleName
                       << " | trackID=" << trackID
                       << " | parentID=" << parentID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | process=" << processName;
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DEBUG DÉTAILLÉ (optionnel) - désactivé car trop verbeux
    // ═══════════════════════════════════════════════════════════════
    /*
    if (fVerbose && eventID < fVerboseMaxEvents) {
        G4ThreeVector prePos = preStepPoint->GetPosition();
        G4ThreeVector postPos = postStepPoint->GetPosition();

        if (parentID == 0 && particleName == "gamma") {
            std::stringstream ss;
            ss << "STEP | trackID=" << trackID
               << " | " << preVolumeName << " -> " << postVolumeName
               << " | z: " << prePos.z()/mm << " -> " << postPos.z()/mm << " mm"
               << " | E=" << kineticEnergy/keV << " keV";
            Logger::GetInstance()->LogLine(ss.str());
        }
    }
    */
}
