//
// ********************************************************************
// * DetectorConstruction.cc                                          *
// * Géométrie "puits" cylindrique pour dosimétrie Eu-152             *
// * Récipient PMMA avec couronnes d'eau concentriques                *
// ********************************************************************
//
// Configuration géométrique :
//
//   Source Eu-152 (z = 0)
//        |
//        |  20 mm
//        v
//   ┌─────────────────────┐ ← Base ouverte du puits (z = 20 mm)
//   │                     │
//   │  ═══════════════════│ ← Eau : 4 mm (z = 20 à 24 mm)
//   │  (5 couronnes)      │   - Couronne 0 : r = 0-5 mm
//   │                     │   - Couronne 1 : r = 5-10 mm
//   │                     │   - Couronne 2 : r = 10-15 mm
//   │                     │   - Couronne 3 : r = 15-20 mm
//   │                     │   - Couronne 4 : r = 20-24 mm
//   └─────────────────────┘
//   ███████████████████████ ← Couvercle PMMA : 1 mm (z = 24 à 25 mm)
//
//   Paroi latérale PMMA : épaisseur 1 mm, R_ext = 25 mm, R_int = 24 mm
//
// ********************************************************************

#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <cmath>
#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fAir(nullptr), fPMMA(nullptr), fWater(nullptr),
  fWorld_log(nullptr), fPuitsParoi_log(nullptr), fPuitsCouvercle_log(nullptr),
  fWaterRing0_log(nullptr), fWaterRing1_log(nullptr), fWaterRing2_log(nullptr),
  fWaterRing3_log(nullptr), fWaterRing4_log(nullptr),
  fWorld_phys(nullptr), fPuitsParoi_phys(nullptr), fPuitsCouvercle_phys(nullptr),
  fWaterRing0_phys(nullptr), fWaterRing1_phys(nullptr), fWaterRing2_phys(nullptr),
  fWaterRing3_phys(nullptr), fWaterRing4_phys(nullptr),
  fWaterRing0_mass(0.), fWaterRing1_mass(0.), fWaterRing2_mass(0.),
  fWaterRing3_mass(0.), fWaterRing4_mass(0.)
{
    // =========================================================================
    // PARAMÈTRES GÉOMÉTRIQUES DU PUITS
    // =========================================================================
    
    fPuitsRayonExt      = 25.0*mm;     // Rayon extérieur du récipient
    fEpaisseurParoi     = 1.0*mm;      // Épaisseur des parois PMMA
    fPuitsRayonInt      = fPuitsRayonExt - fEpaisseurParoi;  // 24 mm
    fPuitsHauteur       = 5.0*mm;      // Hauteur totale du puits
    fEpaisseurCouvercle = 1.0*mm;      // Épaisseur du couvercle supérieur
    fEauEpaisseur       = fPuitsHauteur - fEpaisseurCouvercle;  // 4 mm
    fPuitsPositionZ     = 20.0*mm;     // Position Z de la base du puits
    fLargeurCouronne    = 5.0*mm;      // Largeur radiale des couronnes
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineMaterials()
{
    G4NistManager* nist = G4NistManager::Instance();
    
    // Air
    fAir = nist->FindOrBuildMaterial("G4_AIR");
    
    // PMMA (Plexiglass) pour le récipient
    fPMMA = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    
    // Eau pour les couronnes de mesure
    fWater = nist->FindOrBuildMaterial("G4_WATER");
    
    G4cout << "\n=== MATÉRIAUX ===" << G4endl;
    G4cout << "Air   : " << fAir->GetName() 
           << ", ρ = " << fAir->GetDensity()/(mg/cm3) << " mg/cm³" << G4endl;
    G4cout << "PMMA  : " << fPMMA->GetName() 
           << ", ρ = " << fPMMA->GetDensity()/(g/cm3) << " g/cm³" << G4endl;
    G4cout << "Eau   : " << fWater->GetName() 
           << ", ρ = " << fWater->GetDensity()/(g/cm3) << " g/cm³" << G4endl;
    G4cout << "==================\n" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineVolumes()
{
    // =========================================================================
    // MONDE
    // =========================================================================
    
    G4double world_sizeXY = 10.0*cm;
    G4double world_sizeZ  = 10.0*cm;
    
    G4Box* solidWorld = new G4Box("World",
                                   world_sizeXY/2., world_sizeXY/2., world_sizeZ/2.);
    
    fWorld_log = new G4LogicalVolume(solidWorld, fAir, "WorldLog");
    
    fWorld_phys = new G4PVPlacement(0,                     // Pas de rotation
                                    G4ThreeVector(),       // Position à l'origine
                                    fWorld_log,            // Volume logique
                                    "World",               // Nom
                                    0,                     // Pas de volume mère
                                    false,                 // Pas de boolean
                                    0,                     // Copy number
                                    true);                 // Check overlaps
    
    // Monde invisible
    fWorld_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    
    // =========================================================================
    // RÉCIPIENT PMMA - PAROI LATÉRALE (tube creux, base ouverte)
    // =========================================================================
    
    // La paroi latérale est un tube creux de hauteur = hauteur totale du puits
    // avec rayon intérieur et rayon extérieur
    
    G4Tubs* solidParoi = new G4Tubs("PuitsParoi",
                                     fPuitsRayonInt,           // Rayon intérieur
                                     fPuitsRayonExt,           // Rayon extérieur
                                     fPuitsHauteur/2.,         // Demi-hauteur
                                     0.*deg, 360.*deg);        // Angle complet
    
    fPuitsParoi_log = new G4LogicalVolume(solidParoi, fPMMA, "PuitsParoiLog");
    
    // Position : centre de la paroi à z = base + hauteur/2
    G4double paroiPosZ = fPuitsPositionZ + fPuitsHauteur/2.;
    
    fPuitsParoi_phys = new G4PVPlacement(0,
                                          G4ThreeVector(0., 0., paroiPosZ),
                                          fPuitsParoi_log,
                                          "PuitsParoi",
                                          fWorld_log,
                                          false,
                                          0,
                                          true);
    
    // Visualisation : gris semi-transparent
    G4VisAttributes* paroiVis = new G4VisAttributes(G4Colour(0.7, 0.7, 0.7, 0.4));
    paroiVis->SetForceSolid(true);
    fPuitsParoi_log->SetVisAttributes(paroiVis);
    
    // =========================================================================
    // RÉCIPIENT PMMA - COUVERCLE SUPÉRIEUR (disque plein)
    // =========================================================================
    
    G4Tubs* solidCouvercle = new G4Tubs("PuitsCouvercle",
                                         0.,                      // Rayon intérieur = 0 (plein)
                                         fPuitsRayonExt,          // Rayon extérieur
                                         fEpaisseurCouvercle/2.,  // Demi-épaisseur
                                         0.*deg, 360.*deg);
    
    fPuitsCouvercle_log = new G4LogicalVolume(solidCouvercle, fPMMA, "PuitsCouvercleLog");
    
    // Position : au sommet du puits
    G4double couverclePosZ = fPuitsPositionZ + fPuitsHauteur - fEpaisseurCouvercle/2.;
    
    fPuitsCouvercle_phys = new G4PVPlacement(0,
                                              G4ThreeVector(0., 0., couverclePosZ),
                                              fPuitsCouvercle_log,
                                              "PuitsCouvercle",
                                              fWorld_log,
                                              false,
                                              0,
                                              true);
    
    // Visualisation : gris plus foncé
    G4VisAttributes* couvercleVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.6));
    couvercleVis->SetForceSolid(true);
    fPuitsCouvercle_log->SetVisAttributes(couvercleVis);
    
    // =========================================================================
    // COURONNES D'EAU CONCENTRIQUES
    // =========================================================================
    
    // Position Z commune pour toutes les couronnes (centre de la couche d'eau)
    G4double eauPosZ = fPuitsPositionZ + fEauEpaisseur/2.;
    
    // Couleurs pour les couronnes (dégradé de bleu)
    G4Colour ringColors[5] = {
        G4Colour(0.0, 0.2, 0.8, 0.7),   // Bleu foncé (centre)
        G4Colour(0.0, 0.4, 0.8, 0.7),
        G4Colour(0.0, 0.5, 0.9, 0.7),
        G4Colour(0.2, 0.6, 0.9, 0.7),
        G4Colour(0.3, 0.7, 1.0, 0.7)    // Bleu clair (extérieur)
    };
    
    // Densité de l'eau pour le calcul des masses
    G4double rhoWater = fWater->GetDensity() / (g/cm3);  // 1.0 g/cm³
    
    // --- Couronne 0 : r = 0 à 5 mm (cylindre plein central) ---
    G4double r0_int = 0.0*mm;
    G4double r0_ext = fLargeurCouronne;  // 5 mm
    
    G4Tubs* solidRing0 = new G4Tubs("WaterRing0", r0_int, r0_ext, 
                                     fEauEpaisseur/2., 0.*deg, 360.*deg);
    fWaterRing0_log = new G4LogicalVolume(solidRing0, fWater, "WaterRing0Log");
    fWaterRing0_phys = new G4PVPlacement(0, G4ThreeVector(0., 0., eauPosZ),
                                          fWaterRing0_log, "WaterRing0",
                                          fWorld_log, false, 0, true);
    
    G4double vol0 = pi * (r0_ext*r0_ext - r0_int*r0_int) * fEauEpaisseur;
    fWaterRing0_mass = vol0 * rhoWater / mg;  // en mg
    
    G4VisAttributes* ring0Vis = new G4VisAttributes(ringColors[0]);
    ring0Vis->SetForceSolid(true);
    fWaterRing0_log->SetVisAttributes(ring0Vis);
    
    // --- Couronne 1 : r = 5 à 10 mm ---
    G4double r1_int = 5.0*mm;
    G4double r1_ext = 10.0*mm;
    
    G4Tubs* solidRing1 = new G4Tubs("WaterRing1", r1_int, r1_ext,
                                     fEauEpaisseur/2., 0.*deg, 360.*deg);
    fWaterRing1_log = new G4LogicalVolume(solidRing1, fWater, "WaterRing1Log");
    fWaterRing1_phys = new G4PVPlacement(0, G4ThreeVector(0., 0., eauPosZ),
                                          fWaterRing1_log, "WaterRing1",
                                          fWorld_log, false, 0, true);
    
    G4double vol1 = pi * (r1_ext*r1_ext - r1_int*r1_int) * fEauEpaisseur;
    fWaterRing1_mass = vol1 * rhoWater / mg;
    
    G4VisAttributes* ring1Vis = new G4VisAttributes(ringColors[1]);
    ring1Vis->SetForceSolid(true);
    fWaterRing1_log->SetVisAttributes(ring1Vis);
    
    // --- Couronne 2 : r = 10 à 15 mm ---
    G4double r2_int = 10.0*mm;
    G4double r2_ext = 15.0*mm;
    
    G4Tubs* solidRing2 = new G4Tubs("WaterRing2", r2_int, r2_ext,
                                     fEauEpaisseur/2., 0.*deg, 360.*deg);
    fWaterRing2_log = new G4LogicalVolume(solidRing2, fWater, "WaterRing2Log");
    fWaterRing2_phys = new G4PVPlacement(0, G4ThreeVector(0., 0., eauPosZ),
                                          fWaterRing2_log, "WaterRing2",
                                          fWorld_log, false, 0, true);
    
    G4double vol2 = pi * (r2_ext*r2_ext - r2_int*r2_int) * fEauEpaisseur;
    fWaterRing2_mass = vol2 * rhoWater / mg;
    
    G4VisAttributes* ring2Vis = new G4VisAttributes(ringColors[2]);
    ring2Vis->SetForceSolid(true);
    fWaterRing2_log->SetVisAttributes(ring2Vis);
    
    // --- Couronne 3 : r = 15 à 20 mm ---
    G4double r3_int = 15.0*mm;
    G4double r3_ext = 20.0*mm;
    
    G4Tubs* solidRing3 = new G4Tubs("WaterRing3", r3_int, r3_ext,
                                     fEauEpaisseur/2., 0.*deg, 360.*deg);
    fWaterRing3_log = new G4LogicalVolume(solidRing3, fWater, "WaterRing3Log");
    fWaterRing3_phys = new G4PVPlacement(0, G4ThreeVector(0., 0., eauPosZ),
                                          fWaterRing3_log, "WaterRing3",
                                          fWorld_log, false, 0, true);
    
    G4double vol3 = pi * (r3_ext*r3_ext - r3_int*r3_int) * fEauEpaisseur;
    fWaterRing3_mass = vol3 * rhoWater / mg;
    
    G4VisAttributes* ring3Vis = new G4VisAttributes(ringColors[3]);
    ring3Vis->SetForceSolid(true);
    fWaterRing3_log->SetVisAttributes(ring3Vis);
    
    // --- Couronne 4 : r = 20 à 24 mm (tronquée par la paroi) ---
    G4double r4_int = 20.0*mm;
    G4double r4_ext = fPuitsRayonInt;  // 24 mm (limité par la paroi)
    
    G4Tubs* solidRing4 = new G4Tubs("WaterRing4", r4_int, r4_ext,
                                     fEauEpaisseur/2., 0.*deg, 360.*deg);
    fWaterRing4_log = new G4LogicalVolume(solidRing4, fWater, "WaterRing4Log");
    fWaterRing4_phys = new G4PVPlacement(0, G4ThreeVector(0., 0., eauPosZ),
                                          fWaterRing4_log, "WaterRing4",
                                          fWorld_log, false, 0, true);
    
    G4double vol4 = pi * (r4_ext*r4_ext - r4_int*r4_int) * fEauEpaisseur;
    fWaterRing4_mass = vol4 * rhoWater / mg;
    
    G4VisAttributes* ring4Vis = new G4VisAttributes(ringColors[4]);
    ring4Vis->SetForceSolid(true);
    fWaterRing4_log->SetVisAttributes(ring4Vis);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::PrintParameters()
{
    G4cout << "\n";
    G4cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    G4cout << "║           GÉOMÉTRIE PUITS - COURONNES D'EAU                       ║\n";
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║                                                                   ║\n";
    G4cout << "║  RÉCIPIENT PMMA :                                                 ║\n";
    G4cout << "║    Rayon extérieur      : " << std::setw(6) << fPuitsRayonExt/mm << " mm                            ║\n";
    G4cout << "║    Rayon intérieur      : " << std::setw(6) << fPuitsRayonInt/mm << " mm                            ║\n";
    G4cout << "║    Épaisseur paroi      : " << std::setw(6) << fEpaisseurParoi/mm << " mm                             ║\n";
    G4cout << "║    Hauteur totale       : " << std::setw(6) << fPuitsHauteur/mm << " mm                             ║\n";
    G4cout << "║    Épaisseur couvercle  : " << std::setw(6) << fEpaisseurCouvercle/mm << " mm                             ║\n";
    G4cout << "║    Base                 : OUVERTE                                 ║\n";
    G4cout << "║                                                                   ║\n";
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  POSITIONS SUR L'AXE Z :                                          ║\n";
    G4cout << "║    Source Eu-152        : z = 0 mm                                ║\n";
    G4cout << "║    Base ouverte puits   : z = " << std::setw(5) << fPuitsPositionZ/mm << " mm                           ║\n";
    G4cout << "║    Couche d'eau         : z = " << std::setw(5) << fPuitsPositionZ/mm 
           << " à " << std::setw(5) << (fPuitsPositionZ + fEauEpaisseur)/mm << " mm                    ║\n";
    G4cout << "║    Couvercle PMMA       : z = " << std::setw(5) << (fPuitsPositionZ + fEauEpaisseur)/mm 
           << " à " << std::setw(5) << (fPuitsPositionZ + fPuitsHauteur)/mm << " mm                    ║\n";
    G4cout << "║                                                                   ║\n";
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  COURONNES D'EAU (épaisseur = " << fEauEpaisseur/mm << " mm) :                          ║\n";
    G4cout << "║                                                                   ║\n";
    G4cout << "║    Couronne │  R_int  │  R_ext  │  Volume   │   Masse             ║\n";
    G4cout << "║    ─────────┼─────────┼─────────┼───────────┼──────────           ║\n";
    
    G4double volTotal = 0.;
    G4double massTotal = 0.;
    
    // Couronne 0
    G4double r0_int = 0., r0_ext = 5.;
    G4double vol0 = pi * (r0_ext*r0_ext - r0_int*r0_int) * (fEauEpaisseur/mm);
    G4cout << "║       0     │  " << std::setw(5) << r0_int << "  │  " << std::setw(5) << r0_ext 
           << "  │ " << std::setw(8) << std::fixed << std::setprecision(2) << vol0 
           << "  │ " << std::setw(8) << fWaterRing0_mass << " mg     ║\n";
    volTotal += vol0; massTotal += fWaterRing0_mass;
    
    // Couronne 1
    G4double r1_int = 5., r1_ext = 10.;
    G4double vol1 = pi * (r1_ext*r1_ext - r1_int*r1_int) * (fEauEpaisseur/mm);
    G4cout << "║       1     │  " << std::setw(5) << r1_int << "  │  " << std::setw(5) << r1_ext 
           << "  │ " << std::setw(8) << vol1 
           << "  │ " << std::setw(8) << fWaterRing1_mass << " mg     ║\n";
    volTotal += vol1; massTotal += fWaterRing1_mass;
    
    // Couronne 2
    G4double r2_int = 10., r2_ext = 15.;
    G4double vol2 = pi * (r2_ext*r2_ext - r2_int*r2_int) * (fEauEpaisseur/mm);
    G4cout << "║       2     │  " << std::setw(5) << r2_int << "  │  " << std::setw(5) << r2_ext 
           << "  │ " << std::setw(8) << vol2 
           << "  │ " << std::setw(8) << fWaterRing2_mass << " mg     ║\n";
    volTotal += vol2; massTotal += fWaterRing2_mass;
    
    // Couronne 3
    G4double r3_int = 15., r3_ext = 20.;
    G4double vol3 = pi * (r3_ext*r3_ext - r3_int*r3_int) * (fEauEpaisseur/mm);
    G4cout << "║       3     │  " << std::setw(5) << r3_int << "  │  " << std::setw(5) << r3_ext 
           << "  │ " << std::setw(8) << vol3 
           << "  │ " << std::setw(8) << fWaterRing3_mass << " mg     ║\n";
    volTotal += vol3; massTotal += fWaterRing3_mass;
    
    // Couronne 4 (tronquée)
    G4double r4_int = 20., r4_ext = fPuitsRayonInt/mm;
    G4double vol4 = pi * (r4_ext*r4_ext - r4_int*r4_int) * (fEauEpaisseur/mm);
    G4cout << "║       4     │  " << std::setw(5) << r4_int << "  │  " << std::setw(5) << r4_ext 
           << "  │ " << std::setw(8) << vol4 
           << "  │ " << std::setw(8) << fWaterRing4_mass << " mg     ║\n";
    volTotal += vol4; massTotal += fWaterRing4_mass;
    
    G4cout << "║    ─────────┼─────────┼─────────┼───────────┼──────────           ║\n";
    G4cout << "║    TOTAL    │    0    │  " << std::setw(5) << r4_ext 
           << "  │ " << std::setw(8) << volTotal 
           << "  │ " << std::setw(8) << massTotal << " mg     ║\n";
    G4cout << "║                                                                   ║\n";
    G4cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    G4cout << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // Définition des matériaux
    DefineMaterials();
    
    // Construction des volumes
    DefineVolumes();
    
    // Affichage des paramètres
    PrintParameters();
    
    return fWorld_phys;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
