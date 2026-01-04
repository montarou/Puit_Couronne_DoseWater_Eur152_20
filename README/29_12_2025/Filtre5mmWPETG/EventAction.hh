#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <map>
#include <array>

class RunAction;

class EventAction : public G4UserEventAction
{
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
    
    static const G4int kNbGammaLines = 11;
    
    static G4int GetGammaLineIndex(G4double energy);
    static G4double GetGammaLineEnergy(G4int lineIndex);
    static G4String GetGammaLineName(G4int lineIndex);
    
    struct PrimaryGammaInfo {
        G4int trackID;
        G4double energyInitial;
        G4int gammaLineIndex;
        G4double energyUpstream;
        G4double energyDownstream;
        G4double theta;
        G4double phi;
        G4bool detectedUpstream;
        G4bool detectedDownstream;
        G4bool transmitted;
        G4bool absorbedInFilter;
        G4bool absorbedInWater;
        G4bool exitedFilter;
        G4bool enteredWater;
    };

    struct SecondaryParticleInfo {
        G4int trackID;
        G4int parentID;
        G4int pdgCode;
        G4double energy;
        G4String creatorProcess;
    };

    struct PreContainerPlaneData {
        G4int nPhotons;
        G4double sumEPhotons;
        void Reset() { nPhotons = 0; sumEPhotons = 0.; }
    };
    
    struct PostContainerPlaneData {
        G4int nPhotons_backward;
        G4double sumEPhotons_backward;
        G4int nElectrons_backward;
        G4double sumEElectrons_backward;
        G4int nElectrons_forward;
        G4double sumEElectrons_forward;
        void Reset() {
            nPhotons_backward = 0; sumEPhotons_backward = 0.;
            nElectrons_backward = 0; sumEElectrons_backward = 0.;
            nElectrons_forward = 0; sumEElectrons_forward = 0.;
        }
    };

    void RecordPrimaryUpstream(G4int trackID, G4double energy);
    void RecordPrimaryDownstream(G4int trackID, G4double energy);
    void RecordSecondaryDownstream(G4int trackID, G4int parentID,
                                   G4int pdgCode, G4double energy,
                                   const G4String& process);
    
    void RecordFilterExit(G4int trackID, G4double energy);
    void RecordWaterEntry(G4int trackID, G4double energy);
    void RecordGammaAbsorbed(G4int trackID, const G4String& volumeName);

    void RecordPreContainerPhoton(G4double energy);
    void RecordPostContainerPhotonBackward(G4double energy);
    void RecordPostContainerElectronBackward(G4double energy);
    void RecordPostContainerElectronForward(G4double energy);
    
    const PreContainerPlaneData& GetPreContainerData() const { return fPreContainerData; }
    const PostContainerPlaneData& GetPostContainerData() const { return fPostContainerData; }

    void AddRingEnergy(G4int ringIndex, G4double edep);
    void AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep);
    G4double GetRingEnergy(G4int ringIndex) const;
    G4double GetTotalWaterEnergy() const;

    const std::vector<PrimaryGammaInfo>& GetPrimaryGammas() const { return fPrimaryGammas; }
    const std::vector<SecondaryParticleInfo>& GetSecondariesDownstream() const { return fSecondariesDownstream; }
    G4int GetNumberOfPrimaries() const { return fPrimaryGammas.size(); }
    G4int GetNumberTransmitted() const;
    G4int GetNumberAbsorbed() const;
    G4bool IsPrimaryTrack(G4int trackID) const;
    G4int GetGammaLineForTrack(G4int trackID) const;
    
    const std::array<G4double, DetectorConstruction::kNbWaterRings>& GetRingEnergyDeposit() const {
        return fRingEnergyDeposit;
    }

private:
    RunAction* fRunAction;

    std::vector<PrimaryGammaInfo> fPrimaryGammas;
    std::vector<SecondaryParticleInfo> fSecondariesDownstream;
    std::map<G4int, size_t> fTrackIDtoIndex;

    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingEnergyDeposit;
    std::array<std::array<G4double, kNbGammaLines>, DetectorConstruction::kNbWaterRings> fRingEnergyByLine;

    PreContainerPlaneData fPreContainerData;
    PostContainerPlaneData fPostContainerData;

    G4double fTransmissionTolerance;
    G4int fVerboseLevel;
    
    static const std::array<G4double, kNbGammaLines> kGammaLineEnergies;
    static const std::array<G4String, kNbGammaLines> kGammaLineNames;
};

#endif
