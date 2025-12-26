#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <array>

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
    // NOM DU FICHIER DE SORTIE
    // ═══════════════════════════════════════════════════════════════
    G4String fOutputFileName;
};

#endif
