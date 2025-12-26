#include "EventAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include <cmath>

EventAction::EventAction(RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fTransmissionTolerance(1.0 * keV),
  fVerboseLevel(1)
{
    // Initialiser les dépôts d'énergie par anneau à zéro
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fRingEnergyDeposit[i] = 0.;
    }
}

EventAction::~EventAction()
{}

void EventAction::BeginOfEventAction(const G4Event* event)
{
    // ═══════════════════════════════════════════════════════════════
    // RESET DE TOUTES LES STRUCTURES AU DÉBUT DE CHAQUE ÉVÉNEMENT
    // ═══════════════════════════════════════════════════════════════
    fPrimaryGammas.clear();
    fSecondariesDownstream.clear();
    fTrackIDtoIndex.clear();
    
    // Reset des dépôts d'énergie par anneau
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fRingEnergyDeposit[i] = 0.;
    }

    // ═══════════════════════════════════════════════════════════════
    // RÉCUPÉRATION DES INFORMATIONS DES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    G4int nVertices = event->GetNumberOfPrimaryVertex();
    G4int eventID = event->GetEventID();
    G4int expectedTrackID = 1;

    for (G4int iVertex = 0; iVertex < nVertices; ++iVertex) {
        G4PrimaryVertex* vertex = event->GetPrimaryVertex(iVertex);
        if (!vertex) continue;

        G4PrimaryParticle* primary = vertex->GetPrimary();

        while (primary) {
            G4double energy = primary->GetKineticEnergy();
            G4ThreeVector momentum = primary->GetMomentumDirection();

            G4double theta = std::acos(momentum.z());
            G4double phi = std::atan2(momentum.y(), momentum.x());

            PrimaryGammaInfo info;
            info.trackID = expectedTrackID;
            info.energyInitial = energy;
            info.energyUpstream = 0.;
            info.energyDownstream = 0.;
            info.theta = theta;
            info.phi = phi;
            info.detectedUpstream = false;
            info.detectedDownstream = false;
            info.transmitted = false;

            fTrackIDtoIndex[expectedTrackID] = fPrimaryGammas.size();
            fPrimaryGammas.push_back(info);

            if (fVerboseLevel >= 2 && eventID < 5) {
                G4cout << "  BeginOfEvent | Registered primary gamma: "
                       << "trackID=" << expectedTrackID
                       << ", E=" << energy/keV << " keV"
                       << G4endl;
            }

            expectedTrackID++;
            primary = primary->GetNext();
        }
    }

    if (fVerboseLevel >= 1 && (eventID < 10 || eventID % 10000 == 0)) {
        G4cout << "BeginOfEvent " << eventID
               << " | " << fPrimaryGammas.size() << " primary gamma(s) registered"
               << G4endl;
    }
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    G4int eventID = event->GetEventID();
    G4int nPrimaries = fPrimaryGammas.size();

    std::vector<G4double> primaryEnergies;
    G4double totalEnergy = 0.;
    G4int nTransmitted = 0;
    G4int nAbsorbed = 0;
    G4int nScattered = 0;

    auto analysisManager = G4AnalysisManager::Instance();

    for (size_t i = 0; i < fPrimaryGammas.size(); ++i) {
        const auto& g = fPrimaryGammas[i];

        primaryEnergies.push_back(g.energyInitial);
        totalEnergy += g.energyInitial;

        if (g.transmitted) {
            nTransmitted++;
        } else if (g.detectedUpstream && g.detectedDownstream) {
            nScattered++;
        } else if (g.detectedUpstream && !g.detectedDownstream) {
            nAbsorbed++;
        }

        // Remplir le ntuple GammaData
        analysisManager->FillNtupleIColumn(1, 0, eventID);
        analysisManager->FillNtupleIColumn(1, 1, i);
        analysisManager->FillNtupleDColumn(1, 2, g.energyInitial/keV);
        analysisManager->FillNtupleDColumn(1, 3, g.energyUpstream/keV);
        analysisManager->FillNtupleDColumn(1, 4, g.energyDownstream/keV);
        analysisManager->FillNtupleDColumn(1, 5, g.theta/deg);
        analysisManager->FillNtupleDColumn(1, 6, g.phi/deg);
        analysisManager->FillNtupleIColumn(1, 7, g.detectedUpstream ? 1 : 0);
        analysisManager->FillNtupleIColumn(1, 8, g.detectedDownstream ? 1 : 0);
        analysisManager->FillNtupleIColumn(1, 9, g.transmitted ? 1 : 0);
        analysisManager->AddNtupleRow(1);
    }

    // Calculer la dose totale dans l'eau
    G4double totalWaterDeposit = GetTotalWaterEnergy();

    // ═══════════════════════════════════════════════════════════════
    // REMPLIR LE NTUPLE DOSE PAR ANNEAU (Ntuple 2)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->FillNtupleIColumn(2, 0, eventID);
    analysisManager->FillNtupleIColumn(2, 1, nPrimaries);
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        analysisManager->FillNtupleDColumn(2, 2 + i, fRingEnergyDeposit[i]/keV);
    }
    analysisManager->FillNtupleDColumn(2, 2 + DetectorConstruction::kNbWaterRings, totalWaterDeposit/keV);
    analysisManager->AddNtupleRow(2);

    // Remplir le ntuple EventData
    analysisManager->FillNtupleIColumn(0, 0, eventID);
    analysisManager->FillNtupleIColumn(0, 1, nPrimaries);
    analysisManager->FillNtupleDColumn(0, 2, totalEnergy/keV);
    analysisManager->FillNtupleIColumn(0, 3, nTransmitted);
    analysisManager->FillNtupleIColumn(0, 4, nAbsorbed);
    analysisManager->FillNtupleIColumn(0, 5, nScattered);
    analysisManager->FillNtupleIColumn(0, 6, (G4int)fSecondariesDownstream.size());
    analysisManager->FillNtupleDColumn(0, 7, totalWaterDeposit/keV);
    analysisManager->AddNtupleRow(0);

    // ═══════════════════════════════════════════════════════════════
    // ENVOYER LES STATISTIQUES À RunAction
    // ═══════════════════════════════════════════════════════════════
    if (fRunAction) {
        fRunAction->RecordEventStatistics(nPrimaries, primaryEnergies,
                                          nTransmitted, nAbsorbed,
                                          totalWaterDeposit);
        
        // Envoyer la dose par anneau
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            if (fRingEnergyDeposit[i] > 0.) {
                fRunAction->AddRingEnergy(i, fRingEnergyDeposit[i]);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // AFFICHAGE DE DIAGNOSTIC
    // ═══════════════════════════════════════════════════════════════
    if (fVerboseLevel >= 1 && (eventID < 10 || eventID % 10000 == 0)) {
        G4cout << "\n══════════════════════════════════════════════════" << G4endl;
        G4cout << "EVENT " << eventID << " SUMMARY" << G4endl;
        G4cout << "══════════════════════════════════════════════════" << G4endl;
        G4cout << "Primary gammas: " << nPrimaries << " | Total E: " << totalEnergy/keV << " keV" << G4endl;

        for (size_t i = 0; i < fPrimaryGammas.size(); ++i) {
            const auto& g = fPrimaryGammas[i];
            G4String status = "UNKNOWN";
            if (g.transmitted) status = "TRANSMITTED";
            else if (g.detectedUpstream && g.detectedDownstream) status = "SCATTERED";
            else if (g.detectedUpstream && !g.detectedDownstream) status = "ABSORBED";
            else if (!g.detectedUpstream) status = "MISSED_UPSTREAM";

            G4cout << "  [" << i << "] trackID=" << g.trackID
                   << " E_init=" << g.energyInitial/keV << " keV"
                   << " → [" << status << "]" << G4endl;
        }

        // Afficher la dose par anneau
        G4cout << "Dose dans les anneaux d'eau:" << G4endl;
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            if (fRingEnergyDeposit[i] > 0.) {
                G4cout << "  Anneau " << i << " (r=" 
                       << DetectorConstruction::GetRingInnerRadius(i)/mm << "-"
                       << DetectorConstruction::GetRingOuterRadius(i)/mm << " mm): "
                       << fRingEnergyDeposit[i]/keV << " keV" << G4endl;
            }
        }
        G4cout << "  TOTAL: " << totalWaterDeposit/keV << " keV" << G4endl;
        G4cout << "══════════════════════════════════════════════════\n" << G4endl;
    }
}

void EventAction::RecordPrimaryUpstream(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        size_t index = it->second;
        fPrimaryGammas[index].energyUpstream = energy;
        fPrimaryGammas[index].detectedUpstream = true;
    }
}

void EventAction::RecordPrimaryDownstream(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        size_t index = it->second;
        fPrimaryGammas[index].energyDownstream = energy;
        fPrimaryGammas[index].detectedDownstream = true;

        G4double deltaE = std::abs(fPrimaryGammas[index].energyUpstream - energy);
        if (deltaE < fTransmissionTolerance) {
            fPrimaryGammas[index].transmitted = true;
        }
    }
}

void EventAction::RecordSecondaryDownstream(G4int trackID, G4int parentID,
                                            G4int pdgCode, G4double energy,
                                            const G4String& process)
{
    SecondaryParticleInfo info;
    info.trackID = trackID;
    info.parentID = parentID;
    info.pdgCode = pdgCode;
    info.energy = energy;
    info.creatorProcess = process;

    fSecondariesDownstream.push_back(info);
}

void EventAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingEnergyDeposit[ringIndex] += edep;
    }
}

G4double EventAction::GetRingEnergy(G4int ringIndex) const
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        return fRingEnergyDeposit[ringIndex];
    }
    return 0.;
}

G4double EventAction::GetTotalWaterEnergy() const
{
    G4double total = 0.;
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        total += fRingEnergyDeposit[i];
    }
    return total;
}

G4int EventAction::GetNumberTransmitted() const
{
    G4int count = 0;
    for (const auto& gamma : fPrimaryGammas) {
        if (gamma.transmitted) count++;
    }
    return count;
}

G4int EventAction::GetNumberAbsorbed() const
{
    G4int count = 0;
    for (const auto& gamma : fPrimaryGammas) {
        if (gamma.detectedUpstream && !gamma.detectedDownstream) count++;
    }
    return count;
}

G4bool EventAction::IsPrimaryTrack(G4int trackID) const
{
    return fTrackIDtoIndex.find(trackID) != fTrackIDtoIndex.end();
}
