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
    G4cout << "║  Suivi par raie gamma Eu-152 ACTIVÉ                            ║" << G4endl;
    G4cout << "║  Comptage aux plans PreContainer et PostContainer ACTIVÉ       ║" << G4endl;
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

    // Direction du momentum
    G4ThreeVector momentum = step->GetTrack()->GetMomentumDirection();
    G4double pz = momentum.z();

    // Noms des volumes logiques
    G4String logicalVolumeName = preStepPoint->GetTouchableHandle()
                                             ->GetVolume()
                                             ->GetLogicalVolume()
                                             ->GetName();
    
    G4String postLogVolName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postLogVolName = postStepPoint->GetPhysicalVolume()->GetLogicalVolume()->GetName();
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DE L'ABSORPTION DES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    
    // Si un gamma primaire est tué (absorbé), enregistrer où
    if (parentID == 0 && particleName == "gamma") {
        G4TrackStatus status = track->GetTrackStatus();
        if (status == fStopAndKill || status == fKillTrackAndSecondaries) {
            fEventAction->RecordGammaAbsorbed(trackID, logicalVolumeName);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                std::stringstream ss;
                ss << "GAMMA_ABSORBED | Event " << eventID
                   << " | trackID=" << trackID
                   << " | in " << logicalVolumeName
                   << " | E=" << kineticEnergy/keV << " keV";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DANS LES ANNEAUX D'EAU AVEC SUIVI PAR RAIE
    // ═══════════════════════════════════════════════════════════════

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
                
                // Suivi par raie gamma : identifier la raie du gamma parent
                // Pour les électrons secondaires, trouver le gamma primaire ancêtre
                G4int gammaLineIndex = -1;
                
                if (parentID == 0 && particleName == "gamma") {
                    // C'est un gamma primaire
                    gammaLineIndex = fEventAction->GetGammaLineForTrack(trackID);
                } else {
                    // Particule secondaire - essayer de trouver le gamma primaire parent
                    if (fEventAction->IsPrimaryTrack(parentID)) {
                        gammaLineIndex = fEventAction->GetGammaLineForTrack(parentID);
                    }
                }
                
                // Enregistrer le dépôt par raie si identifié
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
    // COMPTEURS DE VÉRIFICATION (toujours actifs)
    // ═══════════════════════════════════════════════════════════════
    
    // Entrée dans le filtre (gamma primaire)
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
    
    // Sortie du filtre (gamma primaire)
    if (logicalVolumeName == "FilterLog" && postLogVolName != "FilterLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementFilterExit();
            
            // Enregistrer la sortie du filtre pour ce gamma
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
            
            // Enregistrer l'entrée dans l'eau pour ce gamma primaire
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
    // PLANS DE COMPTAGE CYLINDRIQUES (gammas uniquement, direction +z)
    // ═══════════════════════════════════════════════════════════════
    
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
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN PRE-CONTAINER (avant eau, matériau Air)
    // Comptage des photons et électrons vers l'eau (+z)
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PreContainerPlaneLog" && logicalVolumeName != "PreContainerPlaneLog") {
        // Photons vers l'eau (+z)
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPreContainerPlane();
            fEventAction->AddPreContainerPhoton(kineticEnergy);
            
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
        // Électrons vers l'eau (+z)
        if (particleName == "e-" && pz > 0) {
            fEventAction->AddPreContainerElectron(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector pos = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_CONTAINER_PLANE | Event " << eventID
                   << " | ELECTRON +z"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << pos.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN POST-CONTAINER (après eau, matériau W_PETG)
    // Comptage des particules dans les deux sens
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PostContainerPlaneLog" && logicalVolumeName != "PostContainerPlaneLog") {
        
        // --- PHOTONS ---
        if (particleName == "gamma") {
            if (pz > 0) {
                // Photons TRANSMIS vers l'eau (+z) - NOUVEAU
                fRunAction->IncrementPostContainerPlane();
                fEventAction->AddPostContainerPhotonFwd(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector pos = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | PHOTON +z (transmis)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << pos.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            } else {
                // Photons RÉTRODIFFUSÉS depuis l'eau (-z)
                fEventAction->AddPostContainerPhotonBack(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector pos = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | PHOTON -z (backscatter)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << pos.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
        
        // --- ÉLECTRONS ---
        if (particleName == "e-") {
            if (pz > 0) {
                // Électrons vers l'eau (+z)
                fEventAction->AddPostContainerElectronFwd(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector pos = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | ELECTRON +z"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << pos.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            } else {
                // Électrons depuis l'eau (-z)
                fEventAction->AddPostContainerElectronBack(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector pos = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | ELECTRON -z (backscatter)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << pos.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION AU PLAN UPSTREAM
    // ═══════════════════════════════════════════════════════════════

    if (postVolumeName == "UpstreamDetector" && preVolumeName != "UpstreamDetector") {

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
