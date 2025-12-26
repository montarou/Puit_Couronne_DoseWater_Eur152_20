#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Logger.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>
#include <sstream>

RunAction::RunAction()
: G4UserRunAction(),
  fActivity4pi(44000.0),           // 44 kBq
  fConeAngle(20.0*deg),            // Cône de 20° pour optimiser l'irradiation
  fSourcePosZ(2.0*cm),
  fMeanGammasPerDecay(1.924),
  fTotalPrimariesGenerated(0),
  fTotalEventsWithZeroGamma(0),
  fTotalTransmitted(0),
  fTotalAbsorbed(0),
  fTotalEvents(0),
  fTotalWaterEnergy(0.),
  fTotalWaterEventCount(0),
  fGammasEnteringFilter(0),
  fGammasExitingFilter(0),
  fGammasEnteringContainer(0),
  fGammasEnteringWater(0),
  fElectronsInWater(0),
  fGammasPreFilterPlane(0),
  fGammasPostFilterPlane(0),
  fGammasPreWaterPlane(0),
  fGammasPostWaterPlane(0),
  fOutputFileName("puits_couronne_output")
{
    // Initialiser les tableaux
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fRingTotalEnergy[i] = 0.;
        fRingTotalEnergy2[i] = 0.;
        fRingEventCount[i] = 0;
        fRingMasses[i] = 0.;
    }

    // Configuration de G4AnalysisManager
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);
    analysisManager->SetNtupleMerging(true);

    G4cout << "\n╔════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  RunAction initialized - PUITS COURONNE                     ║" << G4endl;
    G4cout << "║  Output file: " << fOutputFileName << ".root" << G4endl;
    G4cout << "║  Dose measurement in " << DetectorConstruction::kNbWaterRings << " water rings" << G4endl;
    G4cout << "╚════════════════════════════════════════════════════════════╝\n" << G4endl;
}

RunAction::~RunAction()
{}

// ═══════════════════════════════════════════════════════════════════════════
// MÉTHODES DE CALCUL POUR LA RENORMALISATION SPATIALE ET TEMPORELLE
// ═══════════════════════════════════════════════════════════════════════════

G4double RunAction::CalculateIrradiationTime(G4int nEvents) const
{
    // ═══════════════════════════════════════════════════════════════
    // CALCUL DU TEMPS D'IRRADIATION ÉQUIVALENT
    // ═══════════════════════════════════════════════════════════════
    //
    // Principe :
    // - La source a une activité A (Bq) sur 4π stéradians
    // - On simule dans un cône de demi-angle θ (fraction f de 4π)
    // - Chaque événement simulé = 1 désintégration dans le cône
    // - Ces N_sim événements correspondent à N_4π = N_sim/f désintégrations
    //   de la source isotrope
    // - Le temps équivalent est T = N_4π / A = N_sim / (f × A)
    //
    // ═══════════════════════════════════════════════════════════════
    
    G4double f = GetSolidAngleFraction();
    
    if (f <= 0. || fActivity4pi <= 0.) {
        return 0.;
    }
    
    G4double T_irr = (G4double)nEvents / (f * fActivity4pi);
    
    return T_irr;  // en secondes
}

G4double RunAction::CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const
{
    // ═══════════════════════════════════════════════════════════════
    // CALCUL DU DÉBIT DE DOSE
    // ═══════════════════════════════════════════════════════════════
    //
    // Débit de dose = Dose totale / Temps d'irradiation
    //               = D_sim / T_irr
    //               = D_sim × f × A / N_sim
    //
    // ═══════════════════════════════════════════════════════════════
    
    G4double T_irr = CalculateIrradiationTime(nEvents);
    
    if (T_irr <= 0.) {
        return 0.;
    }
    
    return totalDose_Gy / T_irr;  // en Gy/s
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    G4cout << "### Run " << run->GetRunID() << " start." << G4endl;

    // ═══════════════════════════════════════════════════════════════
    // OUVRIR LE FICHIER DE LOG
    // ═══════════════════════════════════════════════════════════════
    Logger::GetInstance()->Open("output.log");
    Logger::GetInstance()->SetEchoToConsole(false);  // Pas d'écho console
    
    std::stringstream ss;
    ss << "Run " << run->GetRunID() << " started";
    Logger::GetInstance()->LogHeader(ss.str());

    // Reset des compteurs
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fRingTotalEnergy[i] = 0.;
        fRingTotalEnergy2[i] = 0.;
        fRingEventCount[i] = 0;
    }
    
    fTotalPrimariesGenerated = 0;
    fTotalEventsWithZeroGamma = 0;
    fTotalTransmitted = 0;
    fTotalAbsorbed = 0;
    fTotalEvents = 0;
    fTotalWaterEnergy = 0.;
    fTotalWaterEventCount = 0;
    
    // Reset des compteurs de vérification
    fGammasEnteringFilter = 0;
    fGammasExitingFilter = 0;
    fGammasEnteringContainer = 0;
    fGammasEnteringWater = 0;
    fElectronsInWater = 0;
    
    // Reset des compteurs des plans de comptage
    fGammasPreFilterPlane = 0;
    fGammasPostFilterPlane = 0;
    fGammasPreWaterPlane = 0;
    fGammasPostWaterPlane = 0;

    // Récupérer les masses des anneaux depuis DetectorConstruction
    const DetectorConstruction* detector = 
        static_cast<const DetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    
    if (detector) {
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            fRingMasses[i] = detector->GetRingMass(i);
        }
    }

    // Création des histogrammes et ntuples
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile(fOutputFileName);

    // ═══════════════════════════════════════════════════════════════
    // HISTOGRAMMES
    // ═══════════════════════════════════════════════════════════════

    // H0: Nombre de gammas primaires par événement
    analysisManager->CreateH1("nGammasPerEvent",
                              "Number of primary gammas per event;N_{#gamma};Counts",
                              15, -0.5, 14.5);

    // H1: Spectre des énergies générées
    analysisManager->CreateH1("energySpectrum",
                              "Energy spectrum of generated gammas;E (keV);Counts",
                              1500, 0., 1500.);

    // H2: Énergie totale par événement
    analysisManager->CreateH1("totalEnergyPerEvent",
                              "Total primary energy per event;E_{tot} (keV);Counts",
                              500, 0., 5000.);

    // H3-H7: Dose par anneau (un histogramme par anneau)
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String name = "doseRing" + std::to_string(i);
        G4String title = "Energy deposit in ring " + std::to_string(i) + 
                         " (r=" + std::to_string((int)(DetectorConstruction::GetRingInnerRadius(i)/mm)) +
                         "-" + std::to_string((int)(DetectorConstruction::GetRingOuterRadius(i)/mm)) +
                         " mm);E (keV);Counts";
        analysisManager->CreateH1(name, title, 200, 0., 200.);
    }

    // H8: Dose totale dans l'eau
    analysisManager->CreateH1("doseTotalWater",
                              "Total energy deposit in water;E (keV);Counts",
                              500, 0., 500.);

    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 0: Données par événement
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("EventData", "Event-level data");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("nPrimaries");
    analysisManager->CreateNtupleDColumn("totalEnergy");
    analysisManager->CreateNtupleIColumn("nTransmitted");
    analysisManager->CreateNtupleIColumn("nAbsorbed");
    analysisManager->CreateNtupleIColumn("nScattered");
    analysisManager->CreateNtupleIColumn("nSecondaries");
    analysisManager->CreateNtupleDColumn("totalWaterDeposit");
    analysisManager->FinishNtuple();

    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 1: Données par gamma primaire
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("GammaData", "Primary gamma data");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("gammaIndex");
    analysisManager->CreateNtupleDColumn("energyInitial");
    analysisManager->CreateNtupleDColumn("energyUpstream");
    analysisManager->CreateNtupleDColumn("energyDownstream");
    analysisManager->CreateNtupleDColumn("theta");
    analysisManager->CreateNtupleDColumn("phi");
    analysisManager->CreateNtupleIColumn("detectedUpstream");
    analysisManager->CreateNtupleIColumn("detectedDownstream");
    analysisManager->CreateNtupleIColumn("transmitted");
    analysisManager->FinishNtuple();

    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 2: Dose par anneau (désintégration par désintégration)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("RingDoseData", "Dose per ring per event");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("nPrimaries");
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String colName = "doseRing" + std::to_string(i);
        analysisManager->CreateNtupleDColumn(colName);
    }
    analysisManager->CreateNtupleDColumn("doseTotal");
    analysisManager->FinishNtuple();

    G4cout << "\n╔════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  BeginOfRunAction: Histograms and Ntuples created          ║" << G4endl;
    G4cout << "║  - H0: nGammasPerEvent                                     ║" << G4endl;
    G4cout << "║  - H1: energySpectrum                                      ║" << G4endl;
    G4cout << "║  - H2: totalEnergyPerEvent                                 ║" << G4endl;
    G4cout << "║  - H3-H7: doseRing0 to doseRing4                           ║" << G4endl;
    G4cout << "║  - H8: doseTotalWater                                      ║" << G4endl;
    G4cout << "║  - Ntuple 0: EventData                                     ║" << G4endl;
    G4cout << "║  - Ntuple 1: GammaData                                     ║" << G4endl;
    G4cout << "║  - Ntuple 2: RingDoseData (dose par anneau)                ║" << G4endl;
    G4cout << "╚════════════════════════════════════════════════════════════╝\n" << G4endl;
}

void RunAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingTotalEnergy[ringIndex] += edep;
        fRingTotalEnergy2[ringIndex] += edep * edep;
        fRingEventCount[ringIndex]++;
        
        // Remplir l'histogramme correspondant (H3 à H7)
        auto analysisManager = G4AnalysisManager::Instance();
        analysisManager->FillH1(3 + ringIndex, edep/keV);
    }
    
    // Accumuler aussi le total
    fTotalWaterEnergy += edep;
}

void RunAction::RecordEventStatistics(G4int nPrimaries, 
                                       const std::vector<G4double>& primaryEnergies,
                                       G4int nTransmitted,
                                       G4int nAbsorbed,
                                       G4double totalDeposit)
{
    fTotalEvents++;
    fTotalPrimariesGenerated += nPrimaries;
    fTotalTransmitted += nTransmitted;
    fTotalAbsorbed += nAbsorbed;
    
    if (nPrimaries == 0) {
        fTotalEventsWithZeroGamma++;
    }
    
    if (totalDeposit > 0.) {
        fTotalWaterEventCount++;
    }

    auto analysisManager = G4AnalysisManager::Instance();

    // Remplir les histogrammes
    analysisManager->FillH1(0, nPrimaries);
    
    G4double totalEnergy = 0.;
    for (const auto& e : primaryEnergies) {
        analysisManager->FillH1(1, e/keV);
        totalEnergy += e;
    }
    
    analysisManager->FillH1(2, totalEnergy/keV);
    
    // Dose totale dans l'eau (H8)
    if (totalDeposit > 0.) {
        analysisManager->FillH1(3 + DetectorConstruction::kNbWaterRings, totalDeposit/keV);
    }
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents == 0) return;

    // Fermer le fichier d'analyse
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();

    // ═══════════════════════════════════════════════════════════════
    // CALCULS STATISTIQUES
    // ═══════════════════════════════════════════════════════════════
    
    G4double meanGammasPerEvent = (G4double)fTotalPrimariesGenerated / (G4double)nofEvents;
    G4double fractionZeroGamma = 100.0 * (G4double)fTotalEventsWithZeroGamma / (G4double)nofEvents;
    G4double transmissionRate = (fTotalPrimariesGenerated > 0) ?
        100.0 * fTotalTransmitted / fTotalPrimariesGenerated : 0.;
    G4double absorptionRate = (fTotalPrimariesGenerated > 0) ?
        100.0 * fTotalAbsorbed / fTotalPrimariesGenerated : 0.;

    // ═══════════════════════════════════════════════════════════════
    // RENORMALISATION SPATIALE ET TEMPORELLE
    // ═══════════════════════════════════════════════════════════════
    
    // Fraction d'angle solide du cône
    G4double solidAngleFraction = GetSolidAngleFraction();
    G4double solidAngle_sr = 2.0 * M_PI * (1.0 - std::cos(fConeAngle));
    
    // Nombre équivalent de désintégrations sur 4π
    G4double N_4pi = (G4double)nofEvents / solidAngleFraction;
    
    // Temps d'irradiation équivalent
    G4double simulatedTime_s = CalculateIrradiationTime(nofEvents);
    G4double simulatedTime_min = simulatedTime_s / 60.0;
    G4double simulatedTime_h = simulatedTime_s / 3600.0;
    
    // Temps par événement simulé
    G4double timePerEvent_ms = (nofEvents > 0) ? simulatedTime_s * 1000.0 / nofEvents : 0.;

    // ═══════════════════════════════════════════════════════════════
    // AFFICHAGE DU RÉSUMÉ
    // ═══════════════════════════════════════════════════════════════
    
    G4cout << "\n";
    G4cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    G4cout << "║                    RUN SUMMARY - PUITS COURONNE                   ║\n";
    G4cout << "║              Dose par anneau dans le détecteur eau                ║\n";
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  Number of events processed: " << nofEvents << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  PRIMARY GAMMA GENERATION STATISTICS:                             ║\n";
    G4cout << "║    Total gammas generated     : " << fTotalPrimariesGenerated << G4endl;
    G4cout << "║    Mean gammas per event      : " << meanGammasPerEvent << G4endl;
    G4cout << "║    Expected (theory)          : 1.924" << G4endl;
    G4cout << "║    Events with 0 gamma        : " << fTotalEventsWithZeroGamma
           << " (" << fractionZeroGamma << "%)" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  TRANSMISSION THROUGH FILTER:                                     ║\n";
    G4cout << "║    Gammas transmitted         : " << fTotalTransmitted
           << " (" << transmissionRate << "%)" << G4endl;
    G4cout << "║    Gammas absorbed            : " << fTotalAbsorbed
           << " (" << absorptionRate << "%)" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  ★ COMPTEURS DE VÉRIFICATION (passage dans les volumes):          ║\n";
    G4cout << "║    Gammas entrant dans filtre   : " << fGammasEnteringFilter << G4endl;
    G4cout << "║    Gammas sortant du filtre     : " << fGammasExitingFilter << G4endl;
    G4cout << "║    Gammas entrant dans container: " << fGammasEnteringContainer << G4endl;
    G4cout << "║    Gammas entrant dans l'eau    : " << fGammasEnteringWater << G4endl;
    G4cout << "║    Électrons créés dans l'eau   : " << fElectronsInWater << G4endl;
    if (fGammasEnteringFilter > 0) {
        G4double filterTransmission = 100.0 * fGammasExitingFilter / fGammasEnteringFilter;
        G4cout << "║    Transmission filtre          : " << filterTransmission << "%" << G4endl;
    }
    if (fGammasExitingFilter > 0) {
        G4double waterReach = 100.0 * fGammasEnteringWater / fGammasExitingFilter;
        G4cout << "║    Gammas atteignant l'eau      : " << waterReach << "%" << G4endl;
    }
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  ★ PLANS DE COMPTAGE CYLINDRIQUES :                               ║\n";
    G4cout << "║    Plan pré-filtre              : " << fGammasPreFilterPlane << " gammas" << G4endl;
    G4cout << "║    Plan post-filtre             : " << fGammasPostFilterPlane << " gammas" << G4endl;
    G4cout << "║    Plan pré-eau                 : " << fGammasPreWaterPlane << " gammas" << G4endl;
    G4cout << "║    Plan post-eau                : " << fGammasPostWaterPlane << " gammas" << G4endl;
    if (fGammasPreFilterPlane > 0) {
        G4double transFilter = 100.0 * fGammasPostFilterPlane / fGammasPreFilterPlane;
        G4cout << "║    Transmission filtre (plans)  : " << transFilter << "%" << G4endl;
    }
    if (fGammasPreWaterPlane > 0) {
        G4double transWater = 100.0 * fGammasPostWaterPlane / fGammasPreWaterPlane;
        G4cout << "║    Transmission eau (plans)     : " << transWater << "%" << G4endl;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // SECTION RENORMALISATION SPATIALE ET TEMPORELLE
    // ═══════════════════════════════════════════════════════════════
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  ★★★ RENORMALISATION SPATIALE ET TEMPORELLE ★★★                   ║\n";
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  Paramètres de la source :                                        ║\n";
    G4cout << "║    Activité (4π)              : " << fActivity4pi/1000.0 << " kBq" << G4endl;
    G4cout << "║    Gammas moyens/désint.      : " << fMeanGammasPerDecay << G4endl;
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  Cône d'émission simulé :                                         ║\n";
    G4cout << "║    Demi-angle θ               : " << fConeAngle/deg << "°" << G4endl;
    G4cout << "║    Angle solide Ω             : " << solidAngle_sr << " sr" << G4endl;
    G4cout << "║    Fraction de 4π (f)         : " << solidAngleFraction 
           << " (" << solidAngleFraction*100.0 << "%)" << G4endl;
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  Conversion événements → temps :                                  ║\n";
    G4cout << "║    N_sim (événements)         : " << nofEvents << G4endl;
    G4cout << "║    N_4π équivalent            : " << (G4long)N_4pi << " désintégrations" << G4endl;
    G4cout << "║    Formule : T = N_sim / (f × A)                                  ║\n";
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    G4cout << "║  ══► TEMPS D'IRRADIATION ÉQUIVALENT :                             ║\n";
    G4cout << "║                                                                   ║\n";
    if (simulatedTime_s < 60.0) {
        G4cout << "║         T_irr = " << simulatedTime_s << " secondes                      ║\n";
    } else if (simulatedTime_s < 3600.0) {
        G4cout << "║         T_irr = " << simulatedTime_s << " s = " << simulatedTime_min << " min        ║\n";
    } else {
        G4cout << "║         T_irr = " << simulatedTime_s << " s = " << simulatedTime_h << " h          ║\n";
    }
    G4cout << "║                                                                   ║\n";
    G4cout << "║    (soit " << timePerEvent_ms << " ms par événement simulé)                  ║\n";
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  DOSE DANS LES ANNEAUX D'EAU:                                     ║\n";
    G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    
    G4double totalMass = 0.;
    G4double totalDose_Gy = 0.;
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double rIn = DetectorConstruction::GetRingInnerRadius(i);
        G4double rOut = DetectorConstruction::GetRingOuterRadius(i);
        
        // Dose en Gray
        G4double dose_Gy = 0.;
        if (fRingMasses[i] > 0.) {
            dose_Gy = (fRingTotalEnergy[i] / fRingMasses[i]) / gray;
        }
        
        // Débit de dose en nGy/h
        G4double doseRate_nGyPerH = 0.;
        if (simulatedTime_s > 0.) {
            doseRate_nGyPerH = dose_Gy / simulatedTime_s * 3600.0 * 1.0e9;
        }
        
        // Erreur statistique
        G4double convergence = (fRingEventCount[i] > 0) ?
            100.0 / std::sqrt((G4double)fRingEventCount[i]) : 0.;
        G4double doseRateError = doseRate_nGyPerH * convergence / 100.;
        
        totalMass += fRingMasses[i];
        totalDose_Gy += dose_Gy * fRingMasses[i];  // Pondéré par la masse
        
        G4cout << "║  Anneau " << i << " (r=" << rIn/mm << "-" << rOut/mm << " mm):" << G4endl;
        G4cout << "║    Masse                    : " << fRingMasses[i]/g << " g" << G4endl;
        G4cout << "║    Énergie déposée          : " << fRingTotalEnergy[i]/keV << " keV" << G4endl;
        G4cout << "║    Événements avec dépôt    : " << fRingEventCount[i] << G4endl;
        G4cout << "║    Dose                     : " << dose_Gy*1e9 << " nGy" << G4endl;
        G4cout << "║    Débit de dose            : " << doseRate_nGyPerH << " ± " 
               << doseRateError << " nGy/h (" << convergence << "%)" << G4endl;
        G4cout << "╟───────────────────────────────────────────────────────────────────╢\n";
    }
    
    // Dose totale moyenne (pondérée par masse)
    G4double avgDose_Gy = (totalMass > 0.) ? totalDose_Gy / totalMass : 0.;
    G4double totalDoseRate_nGyPerH = (simulatedTime_s > 0.) ? 
        avgDose_Gy / simulatedTime_s * 3600.0 * 1.0e9 : 0.;
    G4double totalConvergence = (fTotalWaterEventCount > 0) ?
        100.0 / std::sqrt((G4double)fTotalWaterEventCount) : 0.;
    
    G4cout << "║  TOTAL EAU:                                                       ║\n";
    G4cout << "║    Masse totale             : " << totalMass/g << " g" << G4endl;
    G4cout << "║    Énergie totale déposée   : " << fTotalWaterEnergy/keV << " keV" << G4endl;
    G4cout << "║    Événements avec dépôt    : " << fTotalWaterEventCount << G4endl;
    G4cout << "║    Dose moyenne             : " << avgDose_Gy*1e9 << " nGy" << G4endl;
    G4cout << "║    Débit de dose moyen      : " << totalDoseRate_nGyPerH << " nGy/h ("
           << totalConvergence << "%)" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    G4cout << "║  OUTPUT FILE: " << fOutputFileName << ".root" << G4endl;
    G4cout << "║  Contains:                                                        ║\n";
    G4cout << "║    - RingDoseData ntuple: dose par anneau par désintégration      ║\n";
    G4cout << "║    - Histograms: doseRing0 to doseRing4                           ║\n";
    G4cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    G4cout << G4endl;
    
    // ═══════════════════════════════════════════════════════════════
    // ÉCRITURE DU RÉSUMÉ DANS LE FICHIER LOG
    // ═══════════════════════════════════════════════════════════════
    Logger* log = Logger::GetInstance();
    if (log->IsOpen()) {
        log->LogHeader("RUN SUMMARY");
        
        std::stringstream ss;
        ss << "Number of events: " << nofEvents;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Total gammas generated: " << fTotalPrimariesGenerated;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Mean gammas/event: " << meanGammasPerEvent;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Events with 0 gamma: " << fTotalEventsWithZeroGamma 
                       << " (" << fractionZeroGamma << "%)";
        log->LogLine(ss.str());
        
        log->LogLine("");
        log->LogLine("=== COMPTEURS DE VERIFICATION ===");
        
        ss.str(""); ss << "Gammas entrant filtre: " << fGammasEnteringFilter;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Gammas sortant filtre: " << fGammasExitingFilter;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Gammas entrant container: " << fGammasEnteringContainer;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Gammas entrant eau: " << fGammasEnteringWater;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Electrons dans eau: " << fElectronsInWater;
        log->LogLine(ss.str());
        
        if (fGammasEnteringFilter > 0) {
            G4double filterTrans = 100.0 * fGammasExitingFilter / fGammasEnteringFilter;
            ss.str(""); ss << "Transmission filtre: " << filterTrans << "%";
            log->LogLine(ss.str());
        }
        
        log->LogLine("");
        log->LogLine("=== PLANS DE COMPTAGE CYLINDRIQUES ===");
        
        ss.str(""); ss << "Plan pré-filtre: " << fGammasPreFilterPlane << " gammas";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Plan post-filtre: " << fGammasPostFilterPlane << " gammas";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Plan pré-eau: " << fGammasPreWaterPlane << " gammas";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Plan post-eau: " << fGammasPostWaterPlane << " gammas";
        log->LogLine(ss.str());
        
        if (fGammasPreFilterPlane > 0) {
            G4double transFilter = 100.0 * fGammasPostFilterPlane / fGammasPreFilterPlane;
            ss.str(""); ss << "Transmission filtre (plans): " << transFilter << "%";
            log->LogLine(ss.str());
        }
        if (fGammasPreWaterPlane > 0) {
            G4double transWater = 100.0 * fGammasPostWaterPlane / fGammasPreWaterPlane;
            ss.str(""); ss << "Transmission eau (plans): " << transWater << "%";
            log->LogLine(ss.str());
        }
        
        log->LogLine("");
        log->LogLine("=== DOSE PAR ANNEAU ===");
        
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            G4double rIn = DetectorConstruction::GetRingInnerRadius(i);
            G4double rOut = DetectorConstruction::GetRingOuterRadius(i);
            G4double dose_Gy = (fRingMasses[i] > 0.) ? 
                (fRingTotalEnergy[i] / fRingMasses[i]) / gray : 0.;
            G4double doseRate = (simulatedTime_s > 0.) ? 
                dose_Gy / simulatedTime_s * 3600.0 * 1.0e9 : 0.;
            
            ss.str(""); 
            ss << "Ring " << i << " (r=" << rIn/mm << "-" << rOut/mm << " mm): "
               << fRingTotalEnergy[i]/keV << " keV, "
               << fRingEventCount[i] << " events, "
               << doseRate << " nGy/h";
            log->LogLine(ss.str());
        }
        
        log->LogLine("");
        ss.str(""); ss << "TOTAL: " << fTotalWaterEnergy/keV << " keV, "
                       << totalDoseRate_nGyPerH << " nGy/h";
        log->LogLine(ss.str());
        
        log->LogLine("");
        log->LogLine("=== RENORMALISATION SPATIALE ET TEMPORELLE ===");
        
        ss.str(""); ss << "Activité source (4π): " << fActivity4pi/1000.0 << " kBq";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Demi-angle du cône: " << fConeAngle/deg << " deg";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Fraction angle solide (f): " << solidAngleFraction 
                       << " (" << solidAngleFraction*100.0 << "%)";
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Événements simulés (N_sim): " << nofEvents;
        log->LogLine(ss.str());
        
        ss.str(""); ss << "Désintégrations 4π équivalentes: " << (G4long)N_4pi;
        log->LogLine(ss.str());
        
        log->LogLine("");
        ss.str(""); ss << ">>> TEMPS D'IRRADIATION EQUIVALENT: " << simulatedTime_s << " s";
        log->LogLine(ss.str());
        
        if (simulatedTime_s >= 60.0) {
            ss.str(""); ss << "    = " << simulatedTime_min << " min";
            log->LogLine(ss.str());
        }
        if (simulatedTime_s >= 3600.0) {
            ss.str(""); ss << "    = " << simulatedTime_h << " h";
            log->LogLine(ss.str());
        }
        
        ss.str(""); ss << "    (" << timePerEvent_ms << " ms par événement)";
        log->LogLine(ss.str());
        
        log->LogLine("");
        ss.str(""); ss << "Output ROOT file: " << fOutputFileName << ".root";
        log->LogLine(ss.str());
        
        // Fermer le log
        log->Close();
    }
}
