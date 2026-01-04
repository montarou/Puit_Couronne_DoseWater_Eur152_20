#include "EventAction.hh"
#include "RunAction.hh"
#include "Logger.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include <sstream>
#include <cmath>

const std::array<G4double, EventAction::kNbGammaLines> EventAction::kGammaLineEnergies = {
    121.78, 244.70, 344.28, 411.12, 443.97,
    778.90, 867.38, 964.08, 1085.87, 1112.07, 1408.01
};

const std::array<G4String, EventAction::kNbGammaLines> EventAction::kGammaLineNames = {
    "122 keV", "245 keV", "344 keV", "411 keV", "444 keV",
    "779 keV", "867 keV", "964 keV", "1086 keV", "1112 keV", "1408 keV"
};

EventAction::EventAction(RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fTransmissionTolerance(0.01),
  fVerboseLevel(1)
{
    fRingEnergyDeposit.fill(0.);
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    fPreContainerData.Reset();
    fPostContainerData.Reset();
}

EventAction::~EventAction()
{}

G4int EventAction::GetGammaLineIndex(G4double energy)
{
    const G4double tolerance = 0.5 * keV;
    for (G4int i = 0; i < kNbGammaLines; ++i) {
        if (std::abs(energy - kGammaLineEnergies[i] * keV) < tolerance) {
            return i;
        }
    }
    return -1;
}

G4double EventAction::GetGammaLineEnergy(G4int lineIndex)
{
    if (lineIndex >= 0 && lineIndex < kNbGammaLines) {
        return kGammaLineEnergies[lineIndex];
    }
    return 0.;
}

G4String EventAction::GetGammaLineName(G4int lineIndex)
{
    if (lineIndex >= 0 && lineIndex < kNbGammaLines) {
        return kGammaLineNames[lineIndex];
    }
    return "Unknown";
}

void EventAction::BeginOfEventAction(const G4Event* event)
{
    fPrimaryGammas.clear();
    fSecondariesDownstream.clear();
    fTrackIDtoIndex.clear();
    
    fRingEnergyDeposit.fill(0.);
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    fPreContainerData.Reset();
    fPostContainerData.Reset();
    
    G4int nVertices = event->GetNumberOfPrimaryVertex();
    
    for (G4int iv = 0; iv < nVertices; ++iv) {
        G4PrimaryVertex* vertex = event->GetPrimaryVertex(iv);
        G4int nParticles = vertex->GetNumberOfParticle();
        
        for (G4int ip = 0; ip < nParticles; ++ip) {
            G4PrimaryParticle* primary = vertex->GetPrimary(ip);
            
            if (primary->GetPDGcode() == 22) {
                PrimaryGammaInfo info;
                info.trackID = fPrimaryGammas.size() + 1;
                info.energyInitial = primary->GetKineticEnergy();
                info.gammaLineIndex = GetGammaLineIndex(info.energyInitial);
                info.energyUpstream = 0.;
                info.energyDownstream = 0.;
                
                G4ThreeVector mom = primary->GetMomentumDirection();
                info.theta = std::acos(mom.z());
                info.phi = std::atan2(mom.y(), mom.x());
                
                info.detectedUpstream = false;
                info.detectedDownstream = false;
                info.transmitted = false;
                info.absorbedInFilter = false;
                info.absorbedInWater = false;
                info.exitedFilter = false;
                info.enteredWater = false;
                
                fTrackIDtoIndex[info.trackID] = fPrimaryGammas.size();
                fPrimaryGammas.push_back(info);
            }
        }
    }
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    G4int eventID = event->GetEventID();
    
    for (auto& gamma : fPrimaryGammas) {
        if (gamma.detectedUpstream && gamma.detectedDownstream) {
            G4double ratio = gamma.energyDownstream / gamma.energyUpstream;
            gamma.transmitted = (ratio > (1.0 - fTransmissionTolerance));
        }
        
        if (!gamma.detectedDownstream) {
            if (gamma.exitedFilter && !gamma.enteredWater) {
            } else if (!gamma.exitedFilter) {
                gamma.absorbedInFilter = true;
            } else if (gamma.enteredWater) {
                gamma.absorbedInWater = true;
            }
        }
    }
    
    std::vector<G4double> primaryEnergies;
    for (const auto& gamma : fPrimaryGammas) {
        primaryEnergies.push_back(gamma.energyInitial);
        
        if (gamma.gammaLineIndex >= 0) {
            fRunAction->RecordGammaLineStatistics(
                gamma.gammaLineIndex,
                gamma.exitedFilter,
                gamma.absorbedInFilter,
                gamma.enteredWater,
                gamma.absorbedInWater
            );
        }
    }
    
    G4double totalDeposit = 0.;
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        if (fRingEnergyDeposit[i] > 0.) {
            fRunAction->AddRingEnergy(i, fRingEnergyDeposit[i]);
            totalDeposit += fRingEnergyDeposit[i];
            
            for (G4int j = 0; j < kNbGammaLines; ++j) {
                if (fRingEnergyByLine[i][j] > 0.) {
                    fRunAction->AddRingEnergyByLine(i, j, fRingEnergyByLine[i][j]);
                }
            }
        }
    }
    
    fRunAction->RecordEventStatistics(
        fPrimaryGammas.size(),
        primaryEnergies,
        GetNumberTransmitted(),
        GetNumberAbsorbed(),
        totalDeposit,
        fRingEnergyDeposit
    );
    
    fRunAction->RecordPreContainerPlaneData(fPreContainerData);
    fRunAction->RecordPostContainerPlaneData(fPostContainerData);
    
    if (fVerboseLevel > 0 && eventID < 10) {
        std::stringstream ss;
        ss << "EVENT " << eventID << " SUMMARY:";
        ss << " Primaries=" << fPrimaryGammas.size();
        ss << " Transmitted=" << GetNumberTransmitted();
        ss << " Absorbed=" << GetNumberAbsorbed();
        ss << " TotalDeposit=" << totalDeposit/keV << " keV";
        Logger::GetInstance()->LogLine(ss.str());
        
        std::stringstream ssPre;
        ssPre << "  PreContainer: nPhotons=" << fPreContainerData.nPhotons
              << " sumE=" << fPreContainerData.sumEPhotons/keV << " keV";
        Logger::GetInstance()->LogLine(ssPre.str());
        
        std::stringstream ssPost;
        ssPost << "  PostContainer: nPhotons_back=" << fPostContainerData.nPhotons_backward
               << " sumE_back=" << fPostContainerData.sumEPhotons_backward/keV << " keV"
               << " | nElec_back=" << fPostContainerData.nElectrons_backward
               << " sumE_back=" << fPostContainerData.sumEElectrons_backward/keV << " keV"
               << " | nElec_fwd=" << fPostContainerData.nElectrons_forward
               << " sumE_fwd=" << fPostContainerData.sumEElectrons_forward/keV << " keV";
        Logger::GetInstance()->LogLine(ssPost.str());
    }
}

void EventAction::RecordPrimaryUpstream(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        fPrimaryGammas[it->second].detectedUpstream = true;
        fPrimaryGammas[it->second].energyUpstream = energy;
    }
}

void EventAction::RecordPrimaryDownstream(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        fPrimaryGammas[it->second].detectedDownstream = true;
        fPrimaryGammas[it->second].energyDownstream = energy;
    }
}

void EventAction::RecordFilterExit(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        fPrimaryGammas[it->second].exitedFilter = true;
    }
}

void EventAction::RecordWaterEntry(G4int trackID, G4double energy)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        fPrimaryGammas[it->second].enteredWater = true;
    }
}

void EventAction::RecordGammaAbsorbed(G4int trackID, const G4String& volumeName)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        if (volumeName.find("Filter") != std::string::npos) {
            fPrimaryGammas[it->second].absorbedInFilter = true;
        } else if (volumeName.find("Water") != std::string::npos) {
            fPrimaryGammas[it->second].absorbedInWater = true;
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

void EventAction::RecordPreContainerPhoton(G4double energy)
{
    fPreContainerData.nPhotons++;
    fPreContainerData.sumEPhotons += energy;
}

void EventAction::RecordPostContainerPhotonBackward(G4double energy)
{
    fPostContainerData.nPhotons_backward++;
    fPostContainerData.sumEPhotons_backward += energy;
}

void EventAction::RecordPostContainerElectronBackward(G4double energy)
{
    fPostContainerData.nElectrons_backward++;
    fPostContainerData.sumEElectrons_backward += energy;
}

void EventAction::RecordPostContainerElectronForward(G4double energy)
{
    fPostContainerData.nElectrons_forward++;
    fPostContainerData.sumEElectrons_forward += energy;
}

void EventAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingEnergyDeposit[ringIndex] += edep;
    }
}

void EventAction::AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings &&
        lineIndex >= 0 && lineIndex < kNbGammaLines) {
        fRingEnergyByLine[ringIndex][lineIndex] += edep;
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
    for (const auto& edep : fRingEnergyDeposit) {
        total += edep;
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
        if (!gamma.detectedDownstream) count++;
    }
    return count;
}

G4bool EventAction::IsPrimaryTrack(G4int trackID) const
{
    return fTrackIDtoIndex.find(trackID) != fTrackIDtoIndex.end();
}

G4int EventAction::GetGammaLineForTrack(G4int trackID) const
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        return fPrimaryGammas[it->second].gammaLineIndex;
    }
    return -1;
}
