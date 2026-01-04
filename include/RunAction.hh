#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"
#include "EventAction.hh"
#include "globals.hh"
#include <array>
#include <vector>

class G4Run;
class DetectorConstruction;

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();

    virtual void BeginOfRunAction(const G4Run*) override;
    virtual void EndOfRunAction(const G4Run*) override;
    
    // ═══════════════════════════════════════════════════════════════
    // ACCUMULATION DES DONNÉES
    // ═══════════════════════════════════════════════════════════════
    
    void AddRingEnergy(G4int ringIndex, G4double edep);
    void AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep);
    
    // Enregistrement des statistiques par événement
    void RecordEventStatistics(G4int nPrimaries, 
                               const std::vector<G4double>& primaryEnergies,
                               G4int nTransmitted,
                               G4int nAbsorbed,
                               G4double totalDeposit,
                               const std::array<G4double, 5>& ringEnergies);
    
    void RecordGammaLineStatistics(G4int lineIndex,
                                   G4bool exitedFilter,
                                   G4bool absorbedInFilter,
                                   G4bool enteredWater,
                                   G4bool absorbedInWater);
    
    // NOUVEAU : Enregistrement des comptages aux plans container
    void RecordContainerPlaneStatistics(
        G4int preNPhotons, G4double preSumEPhotons,
        G4int preNElectrons, G4double preSumEElectrons,
        G4int postNPhotonsBack, G4double postSumEPhotonsBack,
        G4int postNElectronsBack, G4double postSumEElectronsBack,
        G4int postNPhotonsFwd, G4double postSumEPhotonsFwd,
        G4int postNElectronsFwd, G4double postSumEElectronsFwd
    );
    
    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE PASSAGE
    // ═══════════════════════════════════════════════════════════════
    
    void IncrementFilterEntry() { fGammasEnteringFilter++; }
    void IncrementFilterExit() { fGammasExitingFilter++; }
    void IncrementContainerEntry() { fGammasEnteringContainer++; }
    void IncrementWaterEntry() { fGammasEnteringWater++; }
    void IncrementElectronsInWater() { fElectronsInWater++; }
    void IncrementPreFilterPlane() { fGammasPreFilterPlane++; }
    void IncrementPostFilterPlane() { fGammasPostFilterPlane++; }
    void IncrementPreContainerPlane() { fGammasPreContainerPlane++; }
    void IncrementPostContainerPlane() { fGammasPostContainerPlane++; }
    
    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS
    // ═══════════════════════════════════════════════════════════════
    
    G4int GetLineEmitted(G4int lineIndex) const;
    G4int GetLineAbsorbedFilter(G4int lineIndex) const;
    G4int GetLineAbsorbedWater(G4int lineIndex) const;
    G4double GetLineFilterAbsorptionRate(G4int lineIndex) const;
    G4double GetLineWaterAbsorptionRate(G4int lineIndex) const;
    
    G4double GetRingMass(G4int ringIndex) const { 
        return (ringIndex >= 0 && ringIndex < 5) ? fRingMasses[ringIndex] : 0.; 
    }
    
    // ═══════════════════════════════════════════════════════════════
    // CONVERSION D'UNITÉS
    // ═══════════════════════════════════════════════════════════════
    
    static G4double EnergyToNanoGray(G4double energy_MeV, G4double mass_g);
    static G4double ConvertToNanoGray(G4double energy, G4double mass);
    
    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE NORMALISATION
    // ═══════════════════════════════════════════════════════════════
    
    void SetActivity4pi(G4double activity) { fActivity4pi = activity; }
    void SetConeAngle(G4double angle) { fConeAngle = angle; }
    void SetSourcePosZ(G4double z) { fSourcePosZ = z; }
    
    G4double GetActivity4pi() const { return fActivity4pi; }
    G4double GetConeAngle() const { return fConeAngle; }
    G4double GetSolidAngleFraction() const { return (1. - std::cos(fConeAngle)) / 2.; }
    
    G4double CalculateIrradiationTime(G4int nEvents) const;
    G4double CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const;

private:
    void PrintResults(G4int nEvents) const;
    void PrintDoseTable() const;
    void PrintAbsorptionTable() const;
    
    // Paramètres de normalisation
    G4double fActivity4pi;
    G4double fConeAngle;
    G4double fSourcePosZ;
    G4double fMeanGammasPerDecay;
    
    // Compteurs globaux
    G4int fTotalPrimariesGenerated;
    G4int fTotalEventsWithZeroGamma;
    G4int fTotalTransmitted;
    G4int fTotalAbsorbed;
    G4int fTotalEvents;
    G4double fTotalWaterEnergy;
    G4int fTotalWaterEventCount;
    
    // Compteurs de passage
    G4int fGammasEnteringFilter;
    G4int fGammasExitingFilter;
    G4int fGammasEnteringContainer;
    G4int fGammasEnteringWater;
    G4int fElectronsInWater;
    G4int fGammasPreFilterPlane;
    G4int fGammasPostFilterPlane;
    G4int fGammasPreContainerPlane;
    G4int fGammasPostContainerPlane;
    
    // Données par anneau
    std::array<G4double, 5> fRingTotalEnergy;
    std::array<G4double, 5> fRingTotalEnergy2;
    std::array<G4int, 5> fRingEventCount;
    std::array<G4double, 5> fRingMasses;
    
    // Données par raie gamma
    std::array<std::array<G4double, 11>, 5> fRingEnergyByLine;
    std::array<G4int, 11> fLineEmitted;
    std::array<G4int, 11> fLineExitedFilter;
    std::array<G4int, 11> fLineAbsorbedFilter;
    std::array<G4int, 11> fLineEnteredWater;
    std::array<G4int, 11> fLineAbsorbedWater;
    
    // Nom du fichier de sortie
    G4String fOutputFileName;
};

#endif
