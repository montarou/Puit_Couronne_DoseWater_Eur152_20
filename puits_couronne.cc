//
// ********************************************************************
// * Puits Couronne - Simulation de dose dans l'eau                   *
// * Source Eu-152 + filtre W/PETG + container avec anneaux d'eau     *
// ********************************************************************
//

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"

#include "Randomize.hh"
#include <ctime>

int main(int argc, char** argv)
{
    // ═══════════════════════════════════════════════════════════════
    // INITIALISATION DU GÉNÉRATEUR ALÉATOIRE
    // ═══════════════════════════════════════════════════════════════
    
    G4Random::setTheEngine(new CLHEP::RanecuEngine);
    G4long seed = time(NULL);
    G4Random::setTheSeed(seed);
    
    G4cout << "\n";
    G4cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    G4cout << "║         PUITS COURONNE - Mode Séquentiel                      ║\n";
    G4cout << "║         Dose dans l'eau - Source Eu-152                       ║\n";
    G4cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  Seed aléatoire: " << seed << "                              ║\n";
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    G4cout << G4endl;

    // ═══════════════════════════════════════════════════════════════
    // CRÉATION DU RUN MANAGER (MODE SÉQUENTIEL)
    // ═══════════════════════════════════════════════════════════════
    
    auto* runManager = new G4RunManager;

    // ═══════════════════════════════════════════════════════════════
    // INITIALISATION DES COMPOSANTS OBLIGATOIRES
    // ═══════════════════════════════════════════════════════════════
    
    // Construction du détecteur
    runManager->SetUserInitialization(new DetectorConstruction());
    
    // Liste de physique
    runManager->SetUserInitialization(new PhysicsList());
    
    // Actions utilisateur
    runManager->SetUserInitialization(new ActionInitialization());

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DU MODE INTERACTIF OU BATCH
    // ═══════════════════════════════════════════════════════════════
    
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        // Mode interactif (pas d'arguments)
        ui = new G4UIExecutive(argc, argv);
    }

    // ═══════════════════════════════════════════════════════════════
    // INITIALISATION DE LA VISUALISATION
    // ═══════════════════════════════════════════════════════════════
    
    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();

    // ═══════════════════════════════════════════════════════════════
    // GESTIONNAIRE D'INTERFACE UTILISATEUR
    // ═══════════════════════════════════════════════════════════════
    
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    if (!ui) {
        // Mode batch - exécuter la macro passée en argument
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    }
    else {
        // Mode interactif
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    // ═══════════════════════════════════════════════════════════════
    // NETTOYAGE
    // ═══════════════════════════════════════════════════════════════
    
    delete visManager;
    delete runManager;

    return 0;
}
