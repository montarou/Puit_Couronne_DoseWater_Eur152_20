#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;

/// @brief Construction du détecteur "puits couronne"
///
/// Géométrie :
/// - Source Eu-152 à z = 2 cm (dans PrimaryGeneratorAction)
/// - Filtre cylindrique W/PETG à z = 4 cm
/// - Container cylindrique W/PETG ouvert à z = 10 cm
/// - Tranche PMMA (5 mm) avant l'eau
/// - Anneaux d'eau concentriques à l'intérieur du container
/// - Feuille de tungstène (20 µm) après l'eau


class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();
    
    virtual G4VPhysicalVolume* Construct();

    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS POUR LES VOLUMES SENSIBLES (ANNEAUX D'EAU)
    // ═══════════════════════════════════════════════════════════════

    /// Nombre d'anneaux d'eau (incluant le disque central)
    static const G4int kNbWaterRings = 5;
    
    // Retourne le nom du volume logique pour l'anneau i
    static G4String GetWaterRingName(G4int ringIndex);

    // Retourne le rayon interne de l'anneau i (mm)
    static G4double GetRingInnerRadius(G4int ringIndex);

    /// Retourne le rayon externe de l'anneau i (mm)
    static G4double GetRingOuterRadius(G4int ringIndex);
    
    /// Retourne la masse de l'anneau i (g)
    G4double GetRingMass(G4int ringIndex) const { return fRingMasses[ringIndex]; }

private:

    // ═══════════════════════════════════════════════════════════════
    // MATÉRIAUX
    // ═══════════════════════════════════════════════════════════════
    G4Material* fPETG;
    G4Material* fTungsten;
    G4Material* fW_PETG;
    G4Material* fWater;
    G4Material* fPMMA;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DU FILTRE
    // ═══════════════════════════════════════════════════════════════
    G4double fFilterRadius;
    G4double fFilterThickness;
    G4double fFilterPosZ;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DU CONTAINER
    // ═══════════════════════════════════════════════════════════════
    G4double fContainerInnerRadius;
    G4double fContainerInnerHeight;
    G4double fContainerWallThickness;
    G4double fContainerPosZ;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    G4double fWaterThickness;
    G4double fRingWidth;
    
    // Masses des anneaux (calculées dans Construct())
    std::vector<G4double> fRingMasses;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DES PLANS DE COMPTAGE
    // ═══════════════════════════════════════════════════════════════
    G4double fCountingPlaneThickness;
    G4double fCountingPlaneGap;

    // ═══════════════════════════════════════════════════════════════
    // VOLUMES LOGIQUES DES ANNEAUX (pour identification dans SteppingAction)
    // ═══════════════════════════════════════════════════════════════
    std::vector<G4LogicalVolume*> fWaterRingLogicals;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE LA TRANCHE PMMA (NOUVEAU)
    // ═══════════════════════════════════════════════════════════════
    G4double fPMMAThickness;         // Épaisseur PMMA (5 mm)
    G4double fPMMARadius;            // Rayon PMMA (25 mm)

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE LA FEUILLE DE TUNGSTÈNE (NOUVEAU)
    // ═══════════════════════════════════════════════════════════════
    G4double fTungstenFoilThickness; // Épaisseur feuille W (20 µm)
    G4double fTungstenFoilRadius;    // Rayon feuille W (25 mm)

};

#endif
