#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();
    
    virtual G4VPhysicalVolume* Construct();

    static const G4int kNbWaterRings = 5;
    
    static G4String GetWaterRingName(G4int ringIndex);
    static G4double GetRingInnerRadius(G4int ringIndex);
    static G4double GetRingOuterRadius(G4int ringIndex);
    
    G4double GetRingMass(G4int ringIndex) const { return fRingMasses[ringIndex]; }

private:
    G4Material* fPETG;
    G4Material* fTungsten;
    G4Material* fW_PETG;
    G4Material* fWater;

    G4double fFilterRadius;
    G4double fFilterThickness;
    G4double fFilterPosZ;

    G4double fContainerInnerRadius;
    G4double fContainerInnerHeight;
    G4double fContainerWallThickness;
    G4double fContainerPosZ;

    G4double fWaterThickness;
    G4double fRingWidth;
    
    std::vector<G4double> fRingMasses;

    G4double fCountingPlaneThickness;
    G4double fCountingPlaneGap;

    std::vector<G4LogicalVolume*> fWaterRingLogicals;
};

#endif
