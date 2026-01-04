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
  fVerbose(true),
  fVerboseMaxEvents(10)
{
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fWaterRingNames.insert(DetectorConstruction::GetWaterRingName(i) + "Log");
    }
    
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  SteppingAction: Mode VERBOSE activé pour " << fVerboseMaxEvents << " événements     ║" << G4endl;
    G4cout << "║  Plans de comptage:                                            ║" << G4endl;
    G4cout << "║    - PreFilter, PostFilter (autour du filtre, air)             ║" << G4endl;
    G4cout << "║    - PreContainer  (avant eau, GAP=0, air)                     ║" << G4endl;
    G4cout << "║    - PostContainer (après eau, GAP=0, W_PETG)                  ║" << G4endl;
    G4cout << "║  Ntuples PreContainer/PostContainer: ACTIVÉS                   ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    if (!preStepPoint->GetPhysicalVolume()) {
        return;
    }

    G4String preVolumeName = preStepPoint->GetPhysicalVolume()->GetName();
    
    G4String postVolumeName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postVolumeName = postStepPoint->GetPhysicalVolume()->GetName();
    }

    G4Track* track = step->GetTrack();
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4double kineticEnergy = preStepPoint->GetKineticEnergy();

    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DE L'ABSORPTION DES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    
    if (parentID == 0 && particleName == "gamma") {
        G4TrackStatus status = track->GetTrackStatus();
        if (status == fStopAndKill || status == fKillTrackAndSecondaries) {
            G4LogicalVolume* volume = preStepPoint->GetTouchableHandle()
                                                  ->GetVolume()
                                                  ->GetLogicalVolume();
            G4String logVolName = volume->GetName();
            fEventAction->RecordGammaAbsorbed(trackID, logVolName);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                std::stringstream ss;
                ss << "GAMMA_ABSORBED | Event " << eventID
                   << " | trackID=" << trackID
                   << " | in " << logVolName
                   << " | E=" << kineticEnergy/keV << " keV";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DANS LES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════

    G4LogicalVolume* volume = preStepPoint->GetTouchableHandle()
                                          ->GetVolume()
                                          ->GetLogicalVolume();
    G4String logicalVolumeName = volume->GetName();

    if (fWaterRingNames.find(logicalVolumeName) != fWaterRingNames.end()) {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0.) {
            G4int ringIndex = -1;
            
            for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
                G4String ringLogName = DetectorConstruction::GetWaterRingName(i) + "Log";
                if (logicalVolumeName == ringLogName) {
                    ringIndex = i;
                    break;
                }
            }
            
            if (ringIndex >= 0) {
                fEventAction->AddRingEnergy(ringIndex, edep);
                
                G4int gammaLineIndex = -1;
                
                if (parentID == 0 && particleName == "gamma") {
                    gammaLineIndex = fEventAction->GetGammaLineForTrack(trackID);
                } else {
                    if (fEventAction->IsPrimaryTrack(parentID)) {
                        gammaLineIndex = fEventAction->GetGammaLineForTrack(parentID);
                    }
                }
                
                if (gammaLineIndex >= 0) {
                    fEventAction->AddRingEnergyByLine(ringIndex, gammaLineIndex, edep);
                }
                
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
                    if (gammaLineIndex >= 0) {
                        ss << " | Line=" << EventAction::GetGammaLineName(gammaLineIndex);
                    }
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION
    // ═══════════════════════════════════════════════════════════════
    
    G4String postLogVolName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postLogVolName = postStepPoint->GetPhysicalVolume()->GetLogicalVolume()->GetName();
    }
    
    // Entrée dans le filtre
    if (postLogVolName == "FilterLog" && logicalVolumeName != "FilterLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementFilterEntry();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                G4int lineIdx = fEventAction->GetGammaLineForTrack(trackID);
                std::stringstream ss;
                ss << "FILTER_ENTRY | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV";
                if (lineIdx >= 0) {
                    ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
                }
                ss << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Sortie du filtre
    if (logicalVolumeName == "FilterLog" && postLogVolName != "FilterLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementFilterExit();
            
            G4double postEnergy = postStepPoint->GetKineticEnergy();
            fEventAction->RecordFilterExit(trackID, postEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                G4int lineIdx = fEventAction->GetGammaLineForTrack(trackID);
                std::stringstream ss;
                ss << "FILTER_EXIT | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << postEnergy/keV << " keV";
                if (lineIdx >= 0) {
                    ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
                }
                ss << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // Entrée dans le container
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
            
            if (parentID == 0) {
                fEventAction->RecordWaterEntry(trackID, kineticEnergy);
            }
        }
        
        if (particleName == "e-") {
            fRunAction->IncrementElectronsInWater();
        }
        
        if (fVerbose && eventID < fVerboseMaxEvents) {
            G4ThreeVector pos = postStepPoint->GetPosition();
            G4double radius = std::sqrt(pos.x()*pos.x() + pos.y()*pos.y());
            G4int lineIdx = -1;
            if (parentID == 0 && particleName == "gamma") {
                lineIdx = fEventAction->GetGammaLineForTrack(trackID);
            }
            std::stringstream ss;
            ss << "WATER_ENTRY | Event " << eventID
               << " | " << particleName
               << " | trackID=" << trackID
               << " | parentID=" << parentID
               << " | E=" << kineticEnergy/keV << " keV";
            if (lineIdx >= 0) {
                ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
            }
            ss << " | r=" << radius/mm << " mm"
               << " | z=" << pos.z()/mm << " mm"
               << " | " << postLogVolName;
            Logger::GetInstance()->LogLine(ss.str());
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLANS DE COMPTAGE CYLINDRIQUES
    // ═══════════════════════════════════════════════════════════════
    
    G4ThreeVector momentum = step->GetTrack()->GetMomentumDirection();
    G4double pz = momentum.z();
    
    // ─────────────────────────────────────────────────────────────────
    // Plan pré-filtre (vert)
    // ─────────────────────────────────────────────────────────────────
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
    
    // ─────────────────────────────────────────────────────────────────
    // Plan post-filtre (jaune)
    // ─────────────────────────────────────────────────────────────────
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
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN PRE-CONTAINER - Orange - GAP=0 - Air
    // Juste avant l'eau, enregistre photons vers eau (+z)
    // NTUPLE: precontainer
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PreContainerPlaneLog" && logicalVolumeName != "PreContainerPlaneLog") {
        // Compteur global
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPreContainerPlane();
        }
        
        // Enregistrement pour le ntuple (photons vers eau, +z)
        if (particleName == "gamma" && pz > 0) {
            fEventAction->RecordPreContainerPhoton(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_CONTAINER_PLANE | Event " << eventID
                   << " | PHOTON +z"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN POST-CONTAINER - Violet - GAP=0 - W_PETG
    // Juste après l'eau
    // NTUPLE: postcontainer
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PostContainerPlaneLog" && logicalVolumeName != "PostContainerPlaneLog") {
        // Compteur global (gammas +z seulement)
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPostContainerPlane();
        }
        
        // Photons depuis l'eau (direction -z, pz < 0)
        if (particleName == "gamma" && pz < 0) {
            fEventAction->RecordPostContainerPhotonBackward(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "POST_CONTAINER_PLANE | Event " << eventID
                   << " | PHOTON -z (from water)"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
        
        // Électrons depuis l'eau (direction -z, pz < 0)
        if (particleName == "e-" && pz < 0) {
            fEventAction->RecordPostContainerElectronBackward(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "POST_CONTAINER_PLANE | Event " << eventID
                   << " | ELECTRON -z (from water)"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
        
        // Électrons vers l'eau (direction +z, pz > 0)
        if (particleName == "e-" && pz > 0) {
            fEventAction->RecordPostContainerElectronForward(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "POST_CONTAINER_PLANE | Event " << eventID
                   << " | ELECTRON +z (to water)"
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
                    G4int lineIdx = fEventAction->GetGammaLineForTrack(trackID);
                    std::stringstream ss;
                    ss << "UPSTREAM | Event " << eventID
                       << " | PRIMARY gamma trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV";
                    if (lineIdx >= 0) {
                        ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
                    }
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
                    G4int lineIdx = fEventAction->GetGammaLineForTrack(trackID);
                    std::stringstream ss;
                    ss << "DOWNSTREAM | Event " << eventID
                       << " | PRIMARY gamma trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV";
                    if (lineIdx >= 0) {
                        ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
                    }
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
}
