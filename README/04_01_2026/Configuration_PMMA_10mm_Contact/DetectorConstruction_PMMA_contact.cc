#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4UserLimits.hh"
#include "G4UnitsTable.hh"
#include <cmath>

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fPETG(nullptr),
  fTungsten(nullptr),
  fW_PETG(nullptr),
  fWater(nullptr),
  fFilterRadius(2.5*cm),
  fFilterThickness(5.0*mm),
  fFilterPosZ(4.0*cm),
  fContainerInnerRadius(2.5*cm),
  fContainerInnerHeight(7.0*mm),
  fContainerWallThickness(2.0*mm),
  fContainerPosZ(10.0*cm),
  fWaterThickness(5.0*mm),
  fRingWidth(5.0*mm),
  fPMMAThickness(10.0*mm),           // PMMA 10 mm (1 cm)
  fPMMARadius(25.0*mm),              // rayon PMMA
  fTungstenFoilThickness(20.0*um),   // épaisseur feuille W (20 µm)
  fTungstenFoilRadius(25.0*mm),      // rayon feuille W
  fCountingPlaneThickness(1.0*mm),
  fCountingPlaneGap(1.0*mm)
{
    fRingMasses.resize(kNbWaterRings, 0.);
}

DetectorConstruction::~DetectorConstruction()
{}

G4String DetectorConstruction::GetWaterRingName(G4int ringIndex)
{
    return "WaterRing_" + std::to_string(ringIndex);
}

G4double DetectorConstruction::GetRingInnerRadius(G4int ringIndex)
{
    return ringIndex * 5.0 * mm;
}

G4double DetectorConstruction::GetRingOuterRadius(G4int ringIndex)
{
    return (ringIndex + 1) * 5.0 * mm;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();
    
    // =============================================================================
    // MATÉRIAUX
    // =============================================================================

    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    fWater = nist->FindOrBuildMaterial("G4_WATER");
    
    fTungsten = nist->FindOrBuildMaterial("G4_W");
    fPMMA = nist->FindOrBuildMaterial("G4_PLEXIGLASS");

    G4Element* elC = nist->FindOrBuildElement("C");
    G4Element* elH = nist->FindOrBuildElement("H");
    G4Element* elO = nist->FindOrBuildElement("O");

    fPETG = new G4Material("PETG", 1.27*g/cm3, 3, kStateSolid);
    fPETG->AddElement(elC, 10);
    fPETG->AddElement(elH,  8);
    fPETG->AddElement(elO,  4);

    G4double rhoW = fTungsten->GetDensity();
    G4double rhoPETG = fPETG->GetDensity();
    G4double massFracW = 0.75;
    G4double massFracPETG = 0.25;

    G4double rhoMix = 1.0 / (massFracW/rhoW + massFracPETG/rhoPETG);

    fW_PETG = new G4Material("W_PETG_75_25", rhoMix, 2, kStateSolid);
    fW_PETG->AddMaterial(fTungsten, massFracW);
    fW_PETG->AddMaterial(fPETG, massFracPETG);

    G4cout << "\n=== MATÉRIAUX ===" << G4endl;
    G4cout << "W/PETG (75%/25%) density = " << G4BestUnit(rhoMix, "Volumic Mass") << G4endl;
    G4cout << "Water density = " << G4BestUnit(fWater->GetDensity(), "Volumic Mass") << G4endl;
    G4cout << "PMMA density = " << G4BestUnit(fPMMA->GetDensity(), "Volumic Mass") << G4endl;
    G4cout << "Tungsten density = " << G4BestUnit(fTungsten->GetDensity(), "Volumic Mass") << G4endl;
    G4cout << "================\n" << G4endl;

    // =============================================================================
    // WORLD
    // =============================================================================
    G4double world_size = 50*cm;
    G4Box* solidWorld = new G4Box("World", world_size/2, world_size/2, world_size/2);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
    
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0,
                                G4ThreeVector(),
                                logicWorld,
                                "World",
                                0,
                                false,
                                0);

    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    // =============================================================================
    // ENVELOPPE
    // =============================================================================
    G4Box* solidEnveloppe = new G4Box("Enveloppe", 20*cm, 20*cm, 20*cm);
    G4LogicalVolume* logicEnveloppe = new G4LogicalVolume(solidEnveloppe, air, "Enveloppe");

    new G4PVPlacement(nullptr,
                      G4ThreeVector(),
                      logicEnveloppe,
                      "Enveloppe",
                      logicWorld,
                      false,
                      0,
                      true);

    G4VisAttributes* enveloppeVis = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.05));
    enveloppeVis->SetVisibility(false);
    logicEnveloppe->SetVisAttributes(enveloppeVis);

    // =============================================================================
    // FILTRE CYLINDRIQUE W/PETG à z = 4 cm
    // =============================================================================
    
    G4Tubs* solidFilter = new G4Tubs("Filter",
                                      0.,
                                      fFilterRadius,
                                      fFilterThickness/2,
                                      0.*deg,
                                      360.*deg);

    G4LogicalVolume* logicFilter = new G4LogicalVolume(solidFilter, fW_PETG, "FilterLog");

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, fFilterPosZ),
                      logicFilter,
                      "Filter",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4VisAttributes* filterVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.6, 0.7));
    filterVis->SetForceSolid(true);
    logicFilter->SetVisAttributes(filterVis);

    // =============================================================================
    // PLANS DE COMPTAGE CYLINDRIQUES AUTOUR DU FILTRE
    // =============================================================================
    
    G4Tubs* solidCountingPlane = new G4Tubs("CountingPlane",
                                             0.,
                                             fFilterRadius,
                                             fCountingPlaneThickness/2,
                                             0.*deg, 360.*deg);
    
    G4double filter_front_z = fFilterPosZ - fFilterThickness/2;
    G4double filter_back_z = fFilterPosZ + fFilterThickness/2;
    
    // PLAN PRE-FILTRE (vert)
    G4double preFilterPlane_z = filter_front_z - fCountingPlaneGap - fCountingPlaneThickness/2;
    
    G4LogicalVolume* logicPreFilterPlane = new G4LogicalVolume(solidCountingPlane, air, "PreFilterPlaneLog");
    G4VisAttributes* preFilterVis = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.3));
    preFilterVis->SetForceSolid(true);
    logicPreFilterPlane->SetVisAttributes(preFilterVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, preFilterPlane_z),
                      logicPreFilterPlane,
                      "PreFilterPlane",
                      logicEnveloppe,
                      false, 0, true);
    
    // PLAN POST-FILTRE (jaune)
    G4double postFilterPlane_z = filter_back_z + fCountingPlaneGap + fCountingPlaneThickness/2;
    
    G4LogicalVolume* logicPostFilterPlane = new G4LogicalVolume(solidCountingPlane, air, "PostFilterPlaneLog");
    G4VisAttributes* postFilterVis = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0, 0.3));
    postFilterVis->SetForceSolid(true);
    logicPostFilterPlane->SetVisAttributes(postFilterVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, postFilterPlane_z),
                      logicPostFilterPlane,
                      "PostFilterPlane",
                      logicEnveloppe,
                      false, 0, true);

    // =============================================================================
    // PLANS DE COMPTAGE CARRÉS (UPSTREAM/DOWNSTREAM) - anciens
    // =============================================================================
    G4double detector_thickness = 1.0*mm;
    G4double detector_gap = 2.0*mm;
    G4double detector_size = 8.0*cm;
    
    G4double upstream_detector_z = filter_front_z - detector_gap - detector_thickness/2.0;
    G4double downstream_detector_z = filter_back_z + detector_gap + detector_thickness/2.0;
    
    G4Box* solidDetector = new G4Box("Detector",
                                      detector_size/2.0,
                                      detector_size/2.0,
                                      detector_thickness/2.0);

    G4LogicalVolume* logicUpstreamDetector = new G4LogicalVolume(solidDetector, air, "UpstreamDetector");
    G4VisAttributes* upstreamVisAtt = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.2));
    upstreamVisAtt->SetForceSolid(true);
    logicUpstreamDetector->SetVisAttributes(upstreamVisAtt);

    new G4PVPlacement(0,
                     G4ThreeVector(0., 0., upstream_detector_z),
                     logicUpstreamDetector,
                     "UpstreamDetector",
                     logicEnveloppe,
                     false,
                     0,
                     false);

    G4LogicalVolume* logicDownstreamDetector = new G4LogicalVolume(solidDetector, air, "DownstreamDetector");
    G4VisAttributes* downstreamVisAtt = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0, 0.2));
    downstreamVisAtt->SetForceSolid(true);
    logicDownstreamDetector->SetVisAttributes(downstreamVisAtt);
    
    new G4PVPlacement(0,
                     G4ThreeVector(0., 0., downstream_detector_z),
                     logicDownstreamDetector,
                     "DownstreamDetector",
                     logicEnveloppe,
                     false,
                     0,
                     false);

    // =============================================================================
    // CONTAINER W/PETG "PUITS COURONNE"
    // =============================================================================
    
    G4double containerOuterRadius = fContainerInnerRadius + fContainerWallThickness;
    G4double containerTotalHeight = fContainerInnerHeight + fContainerWallThickness;
    
    G4double containerCenterZ = fContainerPosZ;
    G4double containerBottomZ = containerCenterZ - fContainerInnerHeight/2;
    G4double containerTopExternalZ = containerCenterZ + fContainerInnerHeight/2 + fContainerWallThickness;
    
    // Paroi latérale
    G4Tubs* solidContainerWall = new G4Tubs("ContainerWall",
                                             fContainerInnerRadius,
                                             containerOuterRadius,
                                             fContainerInnerHeight/2,
                                             0.*deg, 360.*deg);

    G4LogicalVolume* logicContainerWall = new G4LogicalVolume(solidContainerWall, fW_PETG, "ContainerWallLog");

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, containerCenterZ),
                      logicContainerWall,
                      "ContainerWall",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    // Fond du container (couvercle supérieur)
    G4double topZ = containerCenterZ + fContainerInnerHeight/2 + fContainerWallThickness/2;
    
    G4Tubs* solidContainerTop = new G4Tubs("ContainerTop",
                                            0.,
                                            containerOuterRadius,
                                            fContainerWallThickness/2,
                                            0.*deg, 360.*deg);

    G4LogicalVolume* logicContainerTop = new G4LogicalVolume(solidContainerTop, fW_PETG, "ContainerTopLog");

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, topZ),
                      logicContainerTop,
                      "ContainerTop",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4VisAttributes* containerVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.45, 0.8));
    containerVis->SetForceSolid(true);
    logicContainerWall->SetVisAttributes(containerVis);
    logicContainerTop->SetVisAttributes(containerVis);

    // =============================================================================
    // CALCUL DES POSITIONS Z DES ÉLÉMENTS DANS LE CONTAINER
    // =============================================================================

    G4double waterTopZ = containerCenterZ + fContainerInnerHeight/2;  // 103.5 mm
    G4double waterCenterZ = waterTopZ - fWaterThickness/2;            // 101.0 mm
    G4double waterBottomZ = waterCenterZ - fWaterThickness/2;         // 98.5 mm

    // =============================================================================
    // NOUVEAU : PMMA EN CONTACT DIRECT AVEC L'EAU
    // Le PMMA est placé juste en dessous de l'eau (contact direct, pas de gap)
    // =============================================================================

    G4double pmmaTopZ = waterBottomZ;                        // Haut du PMMA = bas de l'eau (CONTACT DIRECT)
    G4double pmmaCenterZ = pmmaTopZ - fPMMAThickness/2;      // Centre du PMMA
    G4double pmmaBottomZ = pmmaTopZ - fPMMAThickness;        // Bas du PMMA

    G4Tubs* solidPMMA = new G4Tubs("PMMA",
                                   0.,
                                   fPMMARadius,
                                   fPMMAThickness/2,
                                   0.*deg, 360.*deg);

    G4LogicalVolume* logicPMMA = new G4LogicalVolume(solidPMMA, fPMMA, "PMMALog");

    G4VisAttributes* pmmaVis = new G4VisAttributes(G4Colour(1.0, 0.7, 0.4, 0.6));  // Orange clair
    pmmaVis->SetForceSolid(true);
    logicPMMA->SetVisAttributes(pmmaVis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, pmmaCenterZ),
                      logicPMMA,
                      "PMMA",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4cout << "\n╔═════════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║     PMMA EN CONTACT DIRECT AVEC L'EAU (BUILD-UP)                ║" << G4endl;
    G4cout << "╠═════════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Épaisseur : " << fPMMAThickness/mm << " mm                                          ║" << G4endl;
    G4cout << "║  Rayon : " << fPMMARadius/mm << " mm                                              ║" << G4endl;
    G4cout << "║  Z bas : " << pmmaBottomZ/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z haut : " << pmmaTopZ/mm << " mm  <-- CONTACT DIRECT avec eau            ║" << G4endl;
    G4cout << "║  Z centre : " << pmmaCenterZ/mm << " mm                                          ║" << G4endl;
    G4cout << "╚═════════════════════════════════════════════════════════════════╝\n" << G4endl;


    // =============================================================================
    // ANNEAUX D'EAU À L'INTÉRIEUR DU CONTAINER
    // =============================================================================
    
    G4UserLimits* waterLimits = new G4UserLimits(0.1*mm);
    
    std::vector<G4Colour> ringColors = {
        G4Colour(0.0, 0.3, 1.0, 0.6),
        G4Colour(0.0, 0.4, 1.0, 0.6),
        G4Colour(0.0, 0.5, 1.0, 0.6),
        G4Colour(0.0, 0.6, 1.0, 0.6),
        G4Colour(0.0, 0.7, 1.0, 0.6)
    };

    fWaterRingLogicals.clear();
    G4double waterDensity = fWater->GetDensity();

    G4cout << "\n╔═════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║           ANNEAUX D'EAU (VOLUMES SENSIBLES)               ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Index │ R_in (mm) │ R_out (mm) │ Volume (mm³) │ Mass (g) ║" << G4endl;
    G4cout << "╠════════╪═══════════╪════════════╪══════════════╪══════════╣" << G4endl;

    for (G4int i = 0; i < kNbWaterRings; ++i) {
        G4double rIn = GetRingInnerRadius(i);
        G4double rOut = GetRingOuterRadius(i);
        
        G4String ringName = GetWaterRingName(i);
        
        G4Tubs* solidRing = new G4Tubs(ringName,
                                        rIn,
                                        rOut,
                                        fWaterThickness/2,
                                        0.*deg, 360.*deg);

        G4LogicalVolume* logicRing = new G4LogicalVolume(solidRing, fWater, ringName + "Log");
        logicRing->SetUserLimits(waterLimits);
        
        G4VisAttributes* ringVis = new G4VisAttributes(ringColors[i]);
        ringVis->SetForceSolid(true);
        logicRing->SetVisAttributes(ringVis);

        new G4PVPlacement(nullptr,
                          G4ThreeVector(0, 0, waterCenterZ),
                          logicRing,
                          ringName,
                          logicEnveloppe,
                          false,
                          i,
                          true);

        fWaterRingLogicals.push_back(logicRing);

        G4double ringVolume = M_PI * (rOut*rOut - rIn*rIn) * fWaterThickness;
        G4double ringMass = ringVolume * waterDensity;
        fRingMasses[i] = ringMass;

        char buffer[100];
        sprintf(buffer, "║    %d   │   %5.1f   │    %5.1f   │   %8.2f   │  %6.4f  ║",
                i, rIn/mm, rOut/mm, ringVolume/mm3, ringMass/g);
        G4cout << buffer << G4endl;
    }

    G4double totalWaterMass = 0.;
    for (G4int i = 0; i < kNbWaterRings; ++i) {
        totalWaterMass += fRingMasses[i];
    }

    G4cout << "╠═══════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Masse totale d'eau : " << totalWaterMass/g << " g        ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // FEUILLE DE TUNGSTÈNE (20 µm) APRÈS L'EAU
    // =============================================================================

    G4double tungstenFoilBottomZ = waterTopZ;
    G4double tungstenFoilCenterZ = tungstenFoilBottomZ + fTungstenFoilThickness/2;
    G4double tungstenFoilTopZ = tungstenFoilBottomZ + fTungstenFoilThickness;

    G4Tubs* solidTungstenFoil = new G4Tubs("TungstenFoil",
                                           0.,
                                           fTungstenFoilRadius,
                                           fTungstenFoilThickness/2,
                                           0.*deg, 360.*deg);

    G4LogicalVolume* logicTungstenFoil = new G4LogicalVolume(solidTungstenFoil, fTungsten, "TungstenFoilLog");

    G4VisAttributes* tungstenFoilVis = new G4VisAttributes(G4Colour(0.3, 0.3, 0.3, 0.9));
    tungstenFoilVis->SetForceSolid(true);
    logicTungstenFoil->SetVisAttributes(tungstenFoilVis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, tungstenFoilCenterZ),
                      logicTungstenFoil,
                      "TungstenFoil",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4cout << "\n╔═════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║           FEUILLE DE TUNGSTÈNE                           ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Épaisseur : " << fTungstenFoilThickness/um << " µm       ║" << G4endl;
    G4cout << "║  Rayon : " << fTungstenFoilRadius/mm << " mm              ║" << G4endl;
    G4cout << "║  Z bas : " << tungstenFoilBottomZ/mm << " mm              ║" << G4endl;
    G4cout << "║  Z haut : " << tungstenFoilTopZ/mm << " mm                ║" << G4endl;
    G4cout << "║  Z centre : " << tungstenFoilCenterZ/mm << " mm           ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════╝\n" << G4endl;


    // =============================================================================
    // PLANS DE COMPTAGE AUTOUR DE L'EAU
    // PreContainerPlane : DANS L'EAU (chevauchement autorisé)
    // PostContainerPlane : DANS L'EAU (avant la feuille W)
    // =============================================================================
    
    // ─────────────────────────────────────────────────────────────────────────────
    // PLAN PRE-CONTAINER - Orange - CHEVAUCHE LE DÉBUT DE L'EAU
    // Positionné dans le premier mm de l'eau
    // ─────────────────────────────────────────────────────────────────────────────
    G4double preContainerPlane_z = waterBottomZ + fCountingPlaneThickness/2;  // Dans l'eau !
    
    // Plan de comptage cylindrique pour PreContainer (même rayon que l'eau)
    G4Tubs* solidPreContainerPlane = new G4Tubs("PreContainerPlane",
                                                 0.,
                                                 fContainerInnerRadius,
                                                 fCountingPlaneThickness/2,
                                                 0.*deg, 360.*deg);
    
    G4LogicalVolume* logicPreContainerPlane = new G4LogicalVolume(solidPreContainerPlane, fWater, "PreContainerPlaneLog");
    G4VisAttributes* preContainerVis = new G4VisAttributes(G4Colour(1.0, 0.5, 0.0, 0.4));  // Orange
    preContainerVis->SetForceSolid(true);
    logicPreContainerPlane->SetVisAttributes(preContainerVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, preContainerPlane_z),
                      logicPreContainerPlane,
                      "PreContainerPlane",
                      logicEnveloppe,
                      false, 0, false);  // checkOverlaps = false (chevauchement autorisé)
    
    // ─────────────────────────────────────────────────────────────────────────────
    // PLAN POST-CONTAINER - Violet - DANS L'EAU (avant la feuille W)
    // ─────────────────────────────────────────────────────────────────────────────
    G4double postContainerPlane_z = tungstenFoilBottomZ - fCountingPlaneThickness/2;

    G4Tubs* solidPostContainerPlane = new G4Tubs("PostContainerPlane",
                                                 0.,
                                                 fContainerInnerRadius,
                                                 fCountingPlaneThickness/2,
                                                 0.*deg, 360.*deg);

    G4LogicalVolume* logicPostContainerPlane = new G4LogicalVolume(solidPostContainerPlane, fWater, "PostContainerPlaneLog");
    G4VisAttributes* postContainerVis = new G4VisAttributes(G4Colour(0.5, 0.0, 0.5, 0.5));  // Violet
    postContainerVis->SetForceSolid(true);
    logicPostContainerPlane->SetVisAttributes(postContainerVis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, postContainerPlane_z),
                      logicPostContainerPlane,
                      "PostContainerPlane",
                      logicEnveloppe,
                      false, 0, false);  // checkOverlaps = false

    // =============================================================================
    // AFFICHAGE RÉCAPITULATIF DE LA GÉOMÉTRIE
    // =============================================================================
    
    G4double filterVolume = M_PI * fFilterRadius * fFilterRadius * fFilterThickness;
    G4double filterMass = filterVolume * rhoMix;

    G4double pmmaVolume = M_PI * fPMMARadius * fPMMARadius * fPMMAThickness;
    G4double pmmaMass = pmmaVolume * fPMMA->GetDensity();

    G4double tungstenFoilVolume = M_PI * fTungstenFoilRadius * fTungstenFoilRadius * fTungstenFoilThickness;
    G4double tungstenFoilMass = tungstenFoilVolume * fTungsten->GetDensity();

    G4cout << "\n╔══════════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║      GÉOMÉTRIE : PMMA EN CONTACT DIRECT AVEC L'EAU               ║" << G4endl;
    G4cout << "╠══════════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "║  SOURCE Eu-152 : z = 2 cm                                        ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  FILTRE W/PETG : z = " << (fFilterPosZ-fFilterThickness/2)/mm << " - " << (fFilterPosZ+fFilterThickness/2)/mm << " mm (" << fFilterThickness/mm << " mm)             ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  PMMA : z = " << pmmaBottomZ/mm << " - " << pmmaTopZ/mm << " mm (" << fPMMAThickness/mm << " mm)                       ║" << G4endl;
    G4cout << "║         *** EN CONTACT DIRECT AVEC L'EAU ***                     ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  EAU : z = " << waterBottomZ/mm << " - " << waterTopZ/mm << " mm (" << fWaterThickness/mm << " mm)                       ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  FEUILLE W : z = " << tungstenFoilBottomZ/mm << " - " << tungstenFoilTopZ/mm << " mm (" << fTungstenFoilThickness/um << " µm)            ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  PLANS DE COMPTAGE (CHEVAUCHEMENT AUTORISÉ) :                    ║" << G4endl;
    G4cout << "║    PreContainer (orange) : z = " << (preContainerPlane_z-fCountingPlaneThickness/2)/mm << " - " << (preContainerPlane_z+fCountingPlaneThickness/2)/mm << " mm (DANS EAU)    ║" << G4endl;
    G4cout << "║    PostContainer (violet): z = " << (postContainerPlane_z-fCountingPlaneThickness/2)/mm << " - " << (postContainerPlane_z+fCountingPlaneThickness/2)/mm << " mm (DANS EAU) ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  EMPILEMENT (direction +z) :                                     ║" << G4endl;
    G4cout << "║    1. PMMA        : " << pmmaBottomZ/mm << " - " << pmmaTopZ/mm << " mm                              ║" << G4endl;
    G4cout << "║    2. EAU         : " << waterBottomZ/mm << " - " << waterTopZ/mm << " mm  <-- CONTACT DIRECT        ║" << G4endl;
    G4cout << "║    3. Feuille W   : " << tungstenFoilBottomZ/mm << " - " << tungstenFoilTopZ/mm << " mm                           ║" << G4endl;
    G4cout << "║                                                                  ║" << G4endl;
    G4cout << "╚══════════════════════════════════════════════════════════════════╝\n" << G4endl;

    return physWorld;
}
