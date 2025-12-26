//
// ********************************************************************
// * DetectorConstruction.hh                                          *
// * Géométrie "puits" cylindrique pour dosimétrie Eu-152             *
// * Récipient PMMA avec couronnes d'eau concentriques                *
// ********************************************************************
//

#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "globals.hh"

class G4Box;
class G4Tubs;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();

    // Accesseurs pour les volumes logiques des couronnes d'eau (pour le scoring)
    G4LogicalVolume* GetWaterRing0Log() const { return fWaterRing0_log; }
    G4LogicalVolume* GetWaterRing1Log() const { return fWaterRing1_log; }
    G4LogicalVolume* GetWaterRing2Log() const { return fWaterRing2_log; }
    G4LogicalVolume* GetWaterRing3Log() const { return fWaterRing3_log; }
    G4LogicalVolume* GetWaterRing4Log() const { return fWaterRing4_log; }

    // Accesseurs pour les masses des couronnes (pour calcul de dose)
    G4double GetWaterRing0Mass() const { return fWaterRing0_mass; }
    G4double GetWaterRing1Mass() const { return fWaterRing1_mass; }
    G4double GetWaterRing2Mass() const { return fWaterRing2_mass; }
    G4double GetWaterRing3Mass() const { return fWaterRing3_mass; }
    G4double GetWaterRing4Mass() const { return fWaterRing4_mass; }

    // Accesseur pour le volume logique du monde
    G4LogicalVolume* GetWorldLog() const { return fWorld_log; }

    // Paramètres géométriques (accessibles pour PrimaryGeneratorAction)
    G4double GetPuitsPositionZ() const { return fPuitsPositionZ; }
    G4double GetPuitsRayonExt() const { return fPuitsRayonExt; }

private:
    void DefineMaterials();
    void DefineVolumes();
    void PrintParameters();

    // Matériaux
    G4Material* fAir;
    G4Material* fPMMA;
    G4Material* fWater;

    // Volumes logiques
    G4LogicalVolume* fWorld_log;
    G4LogicalVolume* fPuitsParoi_log;
    G4LogicalVolume* fPuitsCouvercle_log;
    G4LogicalVolume* fWaterRing0_log;
    G4LogicalVolume* fWaterRing1_log;
    G4LogicalVolume* fWaterRing2_log;
    G4LogicalVolume* fWaterRing3_log;
    G4LogicalVolume* fWaterRing4_log;

    // Volumes physiques
    G4VPhysicalVolume* fWorld_phys;
    G4VPhysicalVolume* fPuitsParoi_phys;
    G4VPhysicalVolume* fPuitsCouvercle_phys;
    G4VPhysicalVolume* fWaterRing0_phys;
    G4VPhysicalVolume* fWaterRing1_phys;
    G4VPhysicalVolume* fWaterRing2_phys;
    G4VPhysicalVolume* fWaterRing3_phys;
    G4VPhysicalVolume* fWaterRing4_phys;

    // Masses des couronnes d'eau (en mg)
    G4double fWaterRing0_mass;
    G4double fWaterRing1_mass;
    G4double fWaterRing2_mass;
    G4double fWaterRing3_mass;
    G4double fWaterRing4_mass;

    // Paramètres géométriques du puits
    G4double fPuitsRayonExt;      // Rayon extérieur du puits
    G4double fPuitsRayonInt;      // Rayon intérieur du puits
    G4double fPuitsHauteur;       // Hauteur totale du puits
    G4double fEpaisseurParoi;     // Épaisseur de la paroi latérale
    G4double fEpaisseurCouvercle; // Épaisseur du couvercle
    G4double fEauEpaisseur;       // Épaisseur de la couche d'eau
    G4double fPuitsPositionZ;     // Position Z de la base du puits
    G4double fLargeurCouronne;    // Largeur radiale des couronnes
};

#endif
