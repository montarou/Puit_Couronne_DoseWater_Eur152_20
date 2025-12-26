#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <array>
#include <cmath>

class G4Run;

/// @brief Gestion des runs avec statistiques de dose par anneau
///
/// Cette classe crée les histogrammes et ntuples pour enregistrer
/// la dose déposée dans chaque anneau d'eau, désintégration par désintégration.

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();
    
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);
    
    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR ACCUMULER LES DONNÉES (appelées par EventAction)
    // ═══════════════════════════════════════════════════════════════
    
    /// Ajoute l'énergie déposée dans un anneau spécifique
    void AddRingEnergy(G4int ringIndex, G4double edep);
    
    /// Enregistre les statistiques d'un événement
    void RecordEventStatistics(G4int nPrimaries, 
                               const std::vector<G4double>& primaryEnergies,
                               G4int nTransmitted,
                               G4int nAbsorbed,
                               G4double totalDeposit);
    
    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES DE CONFIGURATION POUR LA RENORMALISATION
    // ═══════════════════════════════════════════════════════════════
    void SetConeAngle(G4double angle) { fConeAngle = angle; }
    G4double GetConeAngle() const { return fConeAngle; }
    
    void SetActivity4pi(G4double activity) { fActivity4pi = activity; }
    G4double GetActivity4pi() const { return fActivity4pi; }
    
    void SetMeanGammasPerDecay(G4double n) { fMeanGammasPerDecay = n; }
    G4double GetMeanGammasPerDecay() const { return fMeanGammasPerDecay; }
    
    /// Calcule la fraction d'angle solide du cône
    G4double GetSolidAngleFraction() const { return (1.0 - std::cos(fConeAngle)) / 2.0; }
    
    /// Calcule le temps d'irradiation équivalent pour N événements (désintégrations)
    G4double CalculateIrradiationTime(G4int nEvents) const;
    
    /// Calcule le débit de dose à partir de la dose totale et du nombre d'événements
    G4double CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const;
    
    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR LES COMPTEURS DE VÉRIFICATION
    // ═══════════════════════════════════════════════════════════════
    void IncrementFilterEntry() { fGammasEnteringFilter++; }
    void IncrementFilterExit() { fGammasExitingFilter++; }
    void IncrementContainerEntry() { fGammasEnteringContainer++; }
    void IncrementWaterEntry() { fGammasEnteringWater++; }
    void IncrementElectronsInWater() { fElectronsInWater++; }
    
    // Compteurs pour les nouveaux plans de comptage cylindriques
    void IncrementPreFilterPlane() { fGammasPreFilterPlane++; }
    void IncrementPostFilterPlane() { fGammasPostFilterPlane++; }
    void IncrementPreWaterPlane() { fGammasPreWaterPlane++; }
    void IncrementPostWaterPlane() { fGammasPostWaterPlane++; }
    
    // Accesseurs pour les compteurs de dose par anneau
    G4double GetRingTotalEnergy(G4int ringIndex) const { 
        return (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) ? 
               fRingTotalEnergy[ringIndex] : 0.; 
    }
    G4int GetRingEventCount(G4int ringIndex) const { 
        return (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) ? 
               fRingEventCount[ringIndex] : 0; 
    }

private:
    // ═══════════════════════════════════════════════════════════════
    // DONNÉES POUR LE CALCUL DE LA DOSE PAR ANNEAU
    // ═══════════════════════════════════════════════════════════════
    
    // Énergie totale déposée dans chaque anneau (MeV)
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingTotalEnergy;
    
    // Somme des énergies² (pour calcul de variance)
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingTotalEnergy2;
    
    // Nombre d'événements avec dépôt dans chaque anneau
    std::array<G4int, DetectorConstruction::kNbWaterRings> fRingEventCount;
    
    // Masse de chaque anneau (g) - sera récupérée de DetectorConstruction
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingMasses;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES SOURCE POUR NORMALISATION
    // ═══════════════════════════════════════════════════════════════
    G4double fActivity4pi;          // Activité source 4π (Bq)
    G4double fConeAngle;            // Demi-angle du cône d'émission
    G4double fSourcePosZ;           // Position Z de la source
    G4double fMeanGammasPerDecay;   // Nombre moyen de gammas par désintégration

    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS POUR STATISTIQUES GLOBALES
    // ═══════════════════════════════════════════════════════════════
    G4int fTotalPrimariesGenerated;
    G4int fTotalEventsWithZeroGamma;
    G4int fTotalTransmitted;
    G4int fTotalAbsorbed;
    G4int fTotalEvents;
    
    // Énergie totale déposée dans toute l'eau
    G4double fTotalWaterEnergy;
    G4int fTotalWaterEventCount;
    
    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION (passage dans les volumes)
    // ═══════════════════════════════════════════════════════════════
    G4int fGammasEnteringFilter;      // Gammas entrant dans le filtre
    G4int fGammasExitingFilter;       // Gammas sortant du filtre
    G4int fGammasEnteringContainer;   // Gammas entrant dans le container
    G4int fGammasEnteringWater;       // Gammas entrant dans l'eau
    G4int fElectronsInWater;          // Électrons créés dans l'eau
    
    // Compteurs pour les plans de comptage cylindriques
    G4int fGammasPreFilterPlane;      // Gammas traversant le plan pré-filtre
    G4int fGammasPostFilterPlane;     // Gammas traversant le plan post-filtre
    G4int fGammasPreWaterPlane;       // Gammas traversant le plan pré-eau
    G4int fGammasPostWaterPlane;      // Gammas traversant le plan post-eau

    // ═══════════════════════════════════════════════════════════════
    // NOM DU FICHIER DE SORTIE
    // ═══════════════════════════════════════════════════════════════
    G4String fOutputFileName;
};

#endif
