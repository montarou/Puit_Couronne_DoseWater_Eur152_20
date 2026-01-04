#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <map>
#include <array>

class RunAction;

/// @brief Gestion des événements avec suivi de la dose par anneau et par raie gamma
///
/// Cette classe gère le cycle de vie d'un événement et stocke :
/// - Les informations de tous les gammas primaires générés
/// - La dose déposée dans chaque anneau d'eau (désintégration par désintégration)
/// - Le suivi par raie gamma Eu-152 pour les taux d'absorption
/// - Les comptages aux plans PreContainer et PostContainer

class EventAction : public G4UserEventAction
{
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
    
    // ═══════════════════════════════════════════════════════════════
    // RAIES GAMMA Eu-152 (pour le suivi par raie)
    // ═══════════════════════════════════════════════════════════════
    static const G4int kNbGammaLines = 11;  // Nombre de raies principales Eu-152
    
    /// Retourne l'index de la raie correspondant à l'énergie (ou -1 si non trouvée)
    static G4int GetGammaLineIndex(G4double energy);
    
    /// Retourne l'énergie nominale de la raie (keV)
    static G4double GetGammaLineEnergy(G4int lineIndex);
    
    /// Retourne le nom de la raie
    static G4String GetGammaLineName(G4int lineIndex);
    
    // ═══════════════════════════════════════════════════════════════
    // STRUCTURE POUR LES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    struct PrimaryGammaInfo {
        G4int trackID;              // Identifiant unique de la trace
        G4double energyInitial;     // Énergie à la génération (MeV)
        G4int gammaLineIndex;       // Index de la raie Eu-152 (-1 si non identifiée)
        G4double energyUpstream;    // Énergie au plan upstream
        G4double energyDownstream;  // Énergie au plan downstream
        G4double theta;             // Angle polaire initial (rad)
        G4double phi;               // Angle azimutal initial (rad)
        G4bool detectedUpstream;    // A traversé le plan upstream ?
        G4bool detectedDownstream;  // A traversé le plan downstream ?
        G4bool transmitted;         // Transmis sans perte d'énergie significative ?
        G4bool absorbedInFilter;    // Absorbé dans le filtre ?
        G4bool absorbedInWater;     // Absorbé dans l'eau ?
        G4bool exitedFilter;        // A traversé le filtre ?
        G4bool enteredWater;        // Est entré dans l'eau ?
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
    
    /// Enregistre qu'un gamma primaire a traversé le filtre
    void RecordFilterExit(G4int trackID, G4double energy);
    
    /// Enregistre qu'un gamma primaire est entré dans l'eau
    void RecordWaterEntry(G4int trackID, G4double energy);
    
    /// Enregistre qu'un gamma primaire a été absorbé (track killed)
    void RecordGammaAbsorbed(G4int trackID, const G4String& volumeName);

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR LA DOSE DANS LES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    
    /// Ajoute l'énergie déposée dans un anneau spécifique
    void AddRingEnergy(G4int ringIndex, G4double edep);
    
    /// Ajoute l'énergie déposée par raie gamma dans un anneau
    void AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep);
    
    /// Retourne l'énergie déposée dans un anneau pour cet événement
    G4double GetRingEnergy(G4int ringIndex) const;
    
    /// Retourne l'énergie totale déposée dans tous les anneaux
    G4double GetTotalWaterEnergy() const;

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR LES PLANS DE COMPTAGE CONTAINER
    // ═══════════════════════════════════════════════════════════════
    
    // PreContainerPlane (photons et électrons vers eau, +z)
    void AddPreContainerPhoton(G4double energy);
    void AddPreContainerElectron(G4double energy);
    
    // PostContainerPlane - particules rétrodiffusées (depuis eau, -z)
    void AddPostContainerPhotonBack(G4double energy);
    void AddPostContainerElectronBack(G4double energy);
    
    // PostContainerPlane - particules vers eau (+z) - NOUVEAU pour photons
    void AddPostContainerPhotonFwd(G4double energy);
    void AddPostContainerElectronFwd(G4double energy);
    
    // Accesseurs pour les comptages PreContainer
    G4int GetPreContainerNPhotons() const { return fPreContainerNPhotons; }
    G4double GetPreContainerSumEPhotons() const { return fPreContainerSumEPhotons; }
    G4int GetPreContainerNElectrons() const { return fPreContainerNElectrons; }
    G4double GetPreContainerSumEElectrons() const { return fPreContainerSumEElectrons; }
    
    // Accesseurs pour les comptages PostContainer (backward, depuis eau)
    G4int GetPostContainerNPhotonsBack() const { return fPostContainerNPhotonsBack; }
    G4double GetPostContainerSumEPhotonsBack() const { return fPostContainerSumEPhotonsBack; }
    G4int GetPostContainerNElectronsBack() const { return fPostContainerNElectronsBack; }
    G4double GetPostContainerSumEElectronsBack() const { return fPostContainerSumEElectronsBack; }
    
    // Accesseurs pour les comptages PostContainer (forward, vers eau) - NOUVEAU pour photons
    G4int GetPostContainerNPhotonsFwd() const { return fPostContainerNPhotonsFwd; }
    G4double GetPostContainerSumEPhotonsFwd() const { return fPostContainerSumEPhotonsFwd; }
    G4int GetPostContainerNElectronsFwd() const { return fPostContainerNElectronsFwd; }
    G4double GetPostContainerSumEElectronsFwd() const { return fPostContainerSumEElectronsFwd; }

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
    
    /// Retourne l'index de la raie pour un trackID donné
    G4int GetGammaLineForTrack(G4int trackID) const;

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
    
    // Dose par anneau ET par raie gamma (pour l'événement courant)
    std::array<std::array<G4double, kNbGammaLines>, DetectorConstruction::kNbWaterRings> fRingEnergyByLine;

    // ═══════════════════════════════════════════════════════════════
    // COMPTAGES AUX PLANS CONTAINER (pour l'événement courant)
    // ═══════════════════════════════════════════════════════════════
    
    // PreContainerPlane (vers eau, +z)
    G4int fPreContainerNPhotons;
    G4double fPreContainerSumEPhotons;
    G4int fPreContainerNElectrons;
    G4double fPreContainerSumEElectrons;
    
    // PostContainerPlane - backward (depuis eau, -z)
    G4int fPostContainerNPhotonsBack;
    G4double fPostContainerSumEPhotonsBack;
    G4int fPostContainerNElectronsBack;
    G4double fPostContainerSumEElectronsBack;
    
    // PostContainerPlane - forward (vers eau, +z) - NOUVEAU pour photons
    G4int fPostContainerNPhotonsFwd;
    G4double fPostContainerSumEPhotonsFwd;
    G4int fPostContainerNElectronsFwd;
    G4double fPostContainerSumEElectronsFwd;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES
    // ═══════════════════════════════════════════════════════════════
    G4double fTransmissionTolerance;
    G4int fVerboseLevel;
    
    // ═══════════════════════════════════════════════════════════════
    // ÉNERGIES DES RAIES Eu-152 (keV) - pour identification
    // ═══════════════════════════════════════════════════════════════
    static const std::array<G4double, kNbGammaLines> kGammaLineEnergies;
    static const std::array<G4String, kNbGammaLines> kGammaLineNames;
};

#endif
