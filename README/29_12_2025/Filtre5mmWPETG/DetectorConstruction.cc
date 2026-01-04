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

    // Fond du container
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
    // ANNEAUX D'EAU À L'INTÉRIEUR DU CONTAINER
    // =============================================================================
    
    G4double waterTopZ = containerCenterZ + fContainerInnerHeight/2;
    G4double waterCenterZ = waterTopZ - fWaterThickness/2;
    G4double waterBottomZ = waterCenterZ - fWaterThickness/2;
    
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

    G4cout << "\n╔═══════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║           ANNEAUX D'EAU (VOLUMES SENSIBLES)                ║" << G4endl;
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
    G4cout << "║  Masse totale d'eau : " << totalWaterMass/g << " g                         ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // PLANS DE COMPTAGE AUTOUR DE L'EAU
    // PreContainerPlane : juste avant l'eau (GAP=0), matériau air
    // PostContainerPlane : juste après l'eau (GAP=0), matériau W_PETG
    // =============================================================================
    
    // ─────────────────────────────────────────────────────────────────────────────
    // PLAN PRE-CONTAINER - Orange - GAP = 0 - Matériau AIR
    // Limite haute = surface basse de l'eau
    // ─────────────────────────────────────────────────────────────────────────────
    G4double preContainerPlane_z = waterBottomZ - fCountingPlaneThickness/2;  // GAP = 0
    
    G4LogicalVolume* logicPreContainerPlane = new G4LogicalVolume(solidCountingPlane, air, "PreContainerPlaneLog");
    G4VisAttributes* preContainerVis = new G4VisAttributes(G4Colour(1.0, 0.5, 0.0, 0.4));  // Orange
    preContainerVis->SetForceSolid(true);
    logicPreContainerPlane->SetVisAttributes(preContainerVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, preContainerPlane_z),
                      logicPreContainerPlane,
                      "PreContainerPlane",
                      logicEnveloppe,
                      false, 0, true);
    
    // ─────────────────────────────────────────────────────────────────────────────
    // PLAN POST-CONTAINER - Violet - GAP = 0 - Matériau W_PETG
    // Limite basse = surface haute de l'eau
    // ─────────────────────────────────────────────────────────────────────────────
    G4double postContainerPlane_z = waterTopZ + fCountingPlaneThickness/2;  // GAP = 0
    
    // Plan de comptage en W_PETG (même matériau que le container)
    G4Tubs* solidPostContainerPlane = new G4Tubs("PostContainerPlane",
                                                  0.,
                                                  fContainerInnerRadius,  // Rayon = rayon intérieur container
                                                  fCountingPlaneThickness/2,
                                                  0.*deg, 360.*deg);
    
    G4LogicalVolume* logicPostContainerPlane = new G4LogicalVolume(solidPostContainerPlane, fW_PETG, "PostContainerPlaneLog");
    G4VisAttributes* postContainerVis = new G4VisAttributes(G4Colour(0.5, 0.0, 0.5, 0.5));  // Violet
    postContainerVis->SetForceSolid(true);
    logicPostContainerPlane->SetVisAttributes(postContainerVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, postContainerPlane_z),
                      logicPostContainerPlane,
                      "PostContainerPlane",
                      logicEnveloppe,
                      false, 0, true);

    // =============================================================================
    // AFFICHAGE RÉCAPITULATIF DE LA GÉOMÉTRIE
    // =============================================================================
    
    G4double filterVolume = M_PI * fFilterRadius * fFilterRadius * fFilterThickness;
    G4double filterMass = filterVolume * rhoMix;

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║        GÉOMÉTRIE : PUITS COURONNE                              ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "║  SOURCE Eu-152 :                                               ║" << G4endl;
    G4cout << "║    Position : z = 2 cm                                         ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╟────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  FILTRE W/PETG (75%/25%) :                                     ║" << G4endl;
    G4cout << "║    Position (centre) : z = " << fFilterPosZ/cm << " cm                           ║" << G4endl;
    G4cout << "║    Diamètre : " << 2*fFilterRadius/cm << " cm                                      ║" << G4endl;
    G4cout << "║    Épaisseur : " << fFilterThickness/mm << " mm                                     ║" << G4endl;
    G4cout << "║    Densité : " << rhoMix/(g/cm3) << " g/cm³                               ║" << G4endl;
    G4cout << "║    Masse : " << filterMass/g << " g                                    ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╟────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  CONTAINER W/PETG (PUITS COURONNE) :                           ║" << G4endl;
    G4cout << "║    Position (centre cavité) : z = " << containerCenterZ/cm << " cm                  ║" << G4endl;
    G4cout << "║    Rayon intérieur : " << fContainerInnerRadius/cm << " cm                           ║" << G4endl;
    G4cout << "║    Hauteur intérieure : " << fContainerInnerHeight/mm << " mm                          ║" << G4endl;
    G4cout << "║    Épaisseur parois : " << fContainerWallThickness/mm << " mm                            ║" << G4endl;
    G4cout << "║    Face inférieure : OUVERTE (vers la source)                  ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╟────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  DÉTECTEUR EAU (dans le container) :                           ║" << G4endl;
    G4cout << "║    Centre Z : " << waterCenterZ/mm << " mm                                   ║" << G4endl;
    G4cout << "║    Z min (bas) : " << waterBottomZ/mm << " mm                                ║" << G4endl;
    G4cout << "║    Z max (haut) : " << waterTopZ/mm << " mm                                 ║" << G4endl;
    G4cout << "║    Épaisseur : " << fWaterThickness/mm << " mm                                      ║" << G4endl;
    G4cout << "║    Rayon : " << fContainerInnerRadius/mm << " mm                                     ║" << G4endl;
    G4cout << "║    Nombre d'anneaux : " << kNbWaterRings << "                                      ║" << G4endl;
    G4cout << "║    Masse totale : " << totalWaterMass/g << " g                                ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╟────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  PLANS DE COMPTAGE :                                           ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "║  Autour du filtre (air) :                                      ║" << G4endl;
    G4cout << "║    PreFilter  (vert)    : z = " << preFilterPlane_z/mm << " mm                     ║" << G4endl;
    G4cout << "║    PostFilter (jaune)   : z = " << postFilterPlane_z/mm << " mm                     ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "║  Autour de l'eau (GAP=0) :                                     ║" << G4endl;
    G4cout << "║    PreContainer (orange, air)    : z = " << preContainerPlane_z/mm << " mm           ║" << G4endl;
    G4cout << "║      -> Limite haute = " << waterBottomZ/mm << " mm (bas eau)                  ║" << G4endl;
    G4cout << "║    PostContainer (violet, W_PETG): z = " << postContainerPlane_z/mm << " mm          ║" << G4endl;
    G4cout << "║      -> Limite basse = " << waterTopZ/mm << " mm (haut eau)                 ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╟────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  PLANS DE COMPTAGE CARRÉS (8x8 cm, anciens) :                  ║" << G4endl;
    G4cout << "║    Upstream : z = " << upstream_detector_z/cm << " cm                            ║" << G4endl;
    G4cout << "║    Downstream : z = " << downstream_detector_z/cm << " cm                          ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    return physWorld;
}
