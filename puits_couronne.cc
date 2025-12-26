//
// ********************************************************************
// * PUITS COURONNE - Simulation de dose dans un détecteur liquide   *
// *                                                                  *
// * Géométrie:                                                       *
// * - Source Eu-152 à z = 2 cm                                       *
// * - Filtre W/PETG (75%/25%) à z = 4 cm                             *
// * - Container W/PETG "puits couronne" à z = 10 cm                  *
// * - Anneaux d'eau concentriques pour mesure de dose                *
// ********************************************************************
//

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "PhysicsList.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "Randomize.hh"

int main(int argc, char** argv)
{
    // Detect interactive mode (if no arguments) and define UI session
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    }

    // Optionally: choose a different Random engine...
    G4Random::setTheEngine(new CLHEP::RanecuEngine);
    G4long seed = time(NULL);
    G4Random::setTheSeed(seed);
    
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║            PUITS COURONNE - Simulation Geant4                  ║" << G4endl;
    G4cout << "║                                                                ║" << G4endl;
    G4cout << "║  Dose measurement in water rings inside W/PETG container       ║" << G4endl;
    G4cout << "║  Random seed: " << seed << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // Construct the run manager
    auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

    // Set mandatory initialization classes
    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization());

    // Initialize visualization
    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();

    // Get the pointer to the User Interface manager
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    // Process macro or start UI session
    if (!ui) {
        // batch mode
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    } else {
        // interactive mode
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    // Job termination
    delete visManager;
    delete runManager;

    return 0;
}
