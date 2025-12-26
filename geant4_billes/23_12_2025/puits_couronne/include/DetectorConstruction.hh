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
/// - Anneaux d'eau concentriques à l'intérieur du container

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
    
    /// Retourne le nom du volume logique pour l'anneau i
    static G4String GetWaterRingName(G4int ringIndex);
    
    /// Retourne le rayon interne de l'anneau i (mm)
    static G4double GetRingInnerRadius(G4int ringIndex);
    
    /// Retourne le rayon externe de l'anneau i (mm)
    static G4double GetRingOuterRadius(G4int ringIndex);
    
    /// Retourne la masse de l'anneau i (g)
    G4double GetRingMass(G4int ringIndex) const { return fRingMasses[ringIndex]; }

private:
    // ═══════════════════════════════════════════════════════════════
    // MATÉRIAUX
    // ═══════════════════════════════════════════════════════════════
    G4Material* fPETG;      // Matériau PETG
    G4Material* fTungsten;  // Matériau Tungstène
    G4Material* fW_PETG;    // Mélange W/PETG 75/25
    G4Material* fWater;     // Eau

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DU FILTRE
    // ═══════════════════════════════════════════════════════════════
    G4double fFilterRadius;     // Rayon du filtre (2.5 cm)
    G4double fFilterThickness;  // Épaisseur du filtre (5 mm)
    G4double fFilterPosZ;       // Position Z du filtre (4 cm)

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DU CONTAINER
    // ═══════════════════════════════════════════════════════════════
    G4double fContainerInnerRadius;  // Rayon intérieur (2.5 cm)
    G4double fContainerInnerHeight;  // Hauteur intérieure (7 mm)
    G4double fContainerWallThickness;// Épaisseur des parois (2 mm)
    G4double fContainerPosZ;         // Position Z du centre (10 cm)

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    G4double fWaterThickness;        // Épaisseur de l'eau (5 mm)
    G4double fRingWidth;             // Largeur radiale des anneaux (5 mm)
    
    // Masses des anneaux (calculées dans Construct())
    std::vector<G4double> fRingMasses;

    // ═══════════════════════════════════════════════════════════════
    // VOLUMES LOGIQUES DES ANNEAUX (pour identification dans SteppingAction)
    // ═══════════════════════════════════════════════════════════════
    std::vector<G4LogicalVolume*> fWaterRingLogicals;
};

#endif
