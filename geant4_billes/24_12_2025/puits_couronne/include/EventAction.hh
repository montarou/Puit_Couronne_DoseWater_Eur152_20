#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <map>
#include <array>

class RunAction;

/// @brief Gestion des événements avec suivi de la dose par anneau
///
/// Cette classe gère le cycle de vie d'un événement et stocke :
/// - Les informations de tous les gammas primaires générés
/// - La dose déposée dans chaque anneau d'eau (désintégration par désintégration)

class EventAction : public G4UserEventAction
{
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
    
    // ═══════════════════════════════════════════════════════════════
    // STRUCTURE POUR LES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    struct PrimaryGammaInfo {
        G4int trackID;              // Identifiant unique de la trace
        G4double energyInitial;     // Énergie à la génération (MeV)
        G4double energyUpstream;    // Énergie au plan upstream
        G4double energyDownstream;  // Énergie au plan downstream
        G4double theta;             // Angle polaire initial (rad)
        G4double phi;               // Angle azimutal initial (rad)
        G4bool detectedUpstream;    // A traversé le plan upstream ?
        G4bool detectedDownstream;  // A traversé le plan downstream ?
        G4bool transmitted;         // Transmis sans perte d'énergie significative ?
    };

    // ═══════════════════════════════════════════════════════════════
    // STRUCTURE POUR LES PARTICULES SECONDAIRES DOWNSTREAM
    // ═══════════════════════════════════════════════════════════════
    struct SecondaryParticleInfo {
        G4int trackID;
        G4int parentID;
        G4int pdgCode;
        G4double energy;
        G4String creatorProcess;
    };

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR ENREGISTRER LES PASSAGES (appelées par SteppingAction)
    // ═══════════════════════════════════════════════════════════════

    void RecordPrimaryUpstream(G4int trackID, G4double energy);
    void RecordPrimaryDownstream(G4int trackID, G4double energy);
    void RecordSecondaryDownstream(G4int trackID, G4int parentID,
                                   G4int pdgCode, G4double energy,
                                   const G4String& process);

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR LA DOSE DANS LES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    
    /// Ajoute l'énergie déposée dans un anneau spécifique
    void AddRingEnergy(G4int ringIndex, G4double edep);
    
    /// Retourne l'énergie déposée dans un anneau pour cet événement
    G4double GetRingEnergy(G4int ringIndex) const;
    
    /// Retourne l'énergie totale déposée dans tous les anneaux
    G4double GetTotalWaterEnergy() const;

    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS
    // ═══════════════════════════════════════════════════════════════
    
    const std::vector<PrimaryGammaInfo>& GetPrimaryGammas() const {
        return fPrimaryGammas;
    }

    const std::vector<SecondaryParticleInfo>& GetSecondariesDownstream() const {
        return fSecondariesDownstream;
    }

    G4int GetNumberOfPrimaries() const { return fPrimaryGammas.size(); }
    G4int GetNumberTransmitted() const;
    G4int GetNumberAbsorbed() const;
    G4bool IsPrimaryTrack(G4int trackID) const;

private:
    RunAction* fRunAction;

    // ═══════════════════════════════════════════════════════════════
    // STOCKAGE DES INFORMATIONS DES PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    std::vector<PrimaryGammaInfo> fPrimaryGammas;
    std::vector<SecondaryParticleInfo> fSecondariesDownstream;
    std::map<G4int, size_t> fTrackIDtoIndex;

    // ═══════════════════════════════════════════════════════════════
    // DOSE PAR ANNEAU (pour l'événement courant)
    // ═══════════════════════════════════════════════════════════════
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingEnergyDeposit;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES
    // ═══════════════════════════════════════════════════════════════
    G4double fTransmissionTolerance;
    G4int fVerboseLevel;
};

#endif
