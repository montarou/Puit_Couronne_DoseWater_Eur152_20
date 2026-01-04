#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Logger.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include <iomanip>
#include <sstream>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
// CONSTANTES POUR LA CONVERSION EN DOSE
// ═══════════════════════════════════════════════════════════════

static const G4double kMeVtoJoule = 1.60218e-13;
static const G4double kNanoGrayFactor = 0.160218;  // nGy per MeV per gram

RunAction::RunAction()
: G4UserRunAction(),
  fActivity4pi(3.7e7),
  fConeAngle(20.*deg),
  fSourcePosZ(2.*cm),
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
  fGammasPreContainerPlane(0),
  fGammasPostContainerPlane(0),
  fOutputFileName("puits_couronne.root")
{
    fRingTotalEnergy.fill(0.);
    fRingTotalEnergy2.fill(0.);
    fRingEventCount.fill(0);
    fRingMasses.fill(0.);
    
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    fLineEmitted.fill(0);
    fLineExitedFilter.fill(0);
    fLineAbsorbedFilter.fill(0);
    fLineEnteredWater.fill(0);
    fLineAbsorbedWater.fill(0);
}

RunAction::~RunAction()
{}

// ═══════════════════════════════════════════════════════════════
// CONVERSION D'UNITÉS
// ═══════════════════════════════════════════════════════════════

G4double RunAction::EnergyToNanoGray(G4double energy_MeV, G4double mass_g)
{
    if (mass_g <= 0.) return 0.;
    return energy_MeV * kNanoGrayFactor / mass_g;
}

G4double RunAction::ConvertToNanoGray(G4double energy, G4double mass)
{
    G4double energy_MeV = energy / MeV;
    G4double mass_g = mass / g;
    return EnergyToNanoGray(energy_MeV, mass_g);
}

// ═══════════════════════════════════════════════════════════════
// DÉBUT ET FIN DE RUN
// ═══════════════════════════════════════════════════════════════

void RunAction::BeginOfRunAction(const G4Run* run)
{
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  DÉBUT DU RUN " << run->GetRunID() << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
    
    Logger::GetInstance()->Open("output.log");
    Logger::GetInstance()->LogHeader("Démarrage du Run " + std::to_string(run->GetRunID()));
    
    // ═══════════════════════════════════════════════════════════════
    // CALCUL DIRECT DES MASSES DES ANNEAUX D'EAU (en grammes)
    // ═══════════════════════════════════════════════════════════════
    
    const G4double waterThickness = 5.0 * mm;
    const G4double ringWidth = 5.0 * mm;
    const G4double waterDensity = 1.0 * g/cm3;
    const G4double pi = CLHEP::pi;
    
    G4cout << "\n=== MASSES DES ANNEAUX D'EAU ===" << G4endl;
    LOG("=== MASSES DES ANNEAUX D'EAU ===");
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double rInner = i * ringWidth;
        G4double rOuter = (i + 1) * ringWidth;
        
        G4double volume = pi * (rOuter*rOuter - rInner*rInner) * waterThickness;
        
        fRingMasses[i] = (volume * waterDensity) / g;
        
        std::ostringstream oss;
        oss << "  Anneau " << i 
            << " : r=[" << rInner/mm << "-" << rOuter/mm << "] mm"
            << " | V=" << std::fixed << std::setprecision(4) << volume/cm3 << " cm³"
            << " | m=" << fRingMasses[i] << " g";
        G4cout << oss.str() << G4endl;
        LOG(oss.str());
    }
    
    G4double totalMass = 0.;
    for (const auto& m : fRingMasses) totalMass += m;
    
    std::ostringstream ossTot;
    ossTot << "  TOTAL : " << totalMass << " g";
    G4cout << ossTot.str() << G4endl;
    LOG(ossTot.str());
    G4cout << "================================\n" << G4endl;
    
    // Réinitialiser les compteurs
    fRingTotalEnergy.fill(0.);
    fRingTotalEnergy2.fill(0.);
    fRingEventCount.fill(0);
    
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    fLineEmitted.fill(0);
    fLineExitedFilter.fill(0);
    fLineAbsorbedFilter.fill(0);
    fLineEnteredWater.fill(0);
    fLineAbsorbedWater.fill(0);
    
    fTotalPrimariesGenerated = 0;
    fTotalEventsWithZeroGamma = 0;
    fTotalTransmitted = 0;
    fTotalAbsorbed = 0;
    fTotalEvents = 0;
    fTotalWaterEnergy = 0.;
    fTotalWaterEventCount = 0;
    
    fGammasEnteringFilter = 0;
    fGammasExitingFilter = 0;
    fGammasEnteringContainer = 0;
    fGammasEnteringWater = 0;
    fElectronsInWater = 0;
    fGammasPreFilterPlane = 0;
    fGammasPostFilterPlane = 0;
    fGammasPreContainerPlane = 0;
    fGammasPostContainerPlane = 0;
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION DE L'ANALYSIS MANAGER
    // ═══════════════════════════════════════════════════════════════
    
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);
    
    analysisManager->OpenFile(fOutputFileName);
    
    // Histogrammes de dose en nGy
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String name = "h_dose_ring" + std::to_string(i);
        G4String title = "Dose par désintégration - Anneau " + std::to_string(i) + " [nGy]";
        analysisManager->CreateH1(name, title, 200, 0., 0.5);  // Gamme ajustée 0-0.5 nGy
    }
    
    // Histogramme dose totale - CORRIGÉ: gamme ajustée pour la somme des anneaux
    analysisManager->CreateH1("h_dose_total", "Dose totale par désintégration (somme anneaux) [nGy]", 200, 0., 0.5);
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4String name = "h_edep_line" + std::to_string(i);
        G4String title = "Énergie déposée - Raie " + EventAction::GetGammaLineName(i) + " [keV]";
        analysisManager->CreateH1(name, title, 200, 0., 100.);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 0 : doses
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("doses", "Doses par désintégration");
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String name = "dose_nGy_ring" + std::to_string(i);
        analysisManager->CreateNtupleDColumn(name);
    }
    
    analysisManager->CreateNtupleDColumn("dose_nGy_total");
    analysisManager->CreateNtupleDColumn("edep_keV_total");
    analysisManager->CreateNtupleIColumn("nPrimaries");
    analysisManager->CreateNtupleIColumn("nTransmitted");
    analysisManager->CreateNtupleIColumn("nAbsorbed");
    
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 1 : gamma_lines
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("gamma_lines", "Statistiques par raie gamma");
    analysisManager->CreateNtupleIColumn("lineIndex");
    analysisManager->CreateNtupleDColumn("energy_keV");
    analysisManager->CreateNtupleIColumn("emitted");
    analysisManager->CreateNtupleIColumn("exitedFilter");
    analysisManager->CreateNtupleIColumn("absorbedFilter");
    analysisManager->CreateNtupleIColumn("enteredWater");
    analysisManager->CreateNtupleIColumn("absorbedWater");
    analysisManager->CreateNtupleDColumn("filterAbsRate");
    analysisManager->CreateNtupleDColumn("waterAbsRate");
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 2 : precontainer (particules vers eau, +z)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("precontainer", "Plan PreContainer - particules vers eau (+z)");
    analysisManager->CreateNtupleIColumn("nPhotons");           // Nombre de photons vers eau
    analysisManager->CreateNtupleDColumn("sumEPhotons_keV");    // Somme des énergies photons (keV)
    analysisManager->CreateNtupleIColumn("nElectrons");         // Nombre d'électrons vers eau
    analysisManager->CreateNtupleDColumn("sumEElectrons_keV");  // Somme des énergies électrons (keV)
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 3 : postcontainer (particules depuis et vers eau)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("postcontainer", "Plan PostContainer - particules depuis/vers eau");
    // Particules rétrodiffusées (depuis eau, -z)
    analysisManager->CreateNtupleIColumn("nPhotons_back");           // Photons depuis eau (-z)
    analysisManager->CreateNtupleDColumn("sumEPhotons_back_keV");    // Énergie photons (-z)
    analysisManager->CreateNtupleIColumn("nElectrons_back");         // Électrons depuis eau (-z)
    analysisManager->CreateNtupleDColumn("sumEElectrons_back_keV");  // Énergie électrons (-z)
    // NOUVEAU : Particules transmises (vers eau, +z)
    analysisManager->CreateNtupleIColumn("nPhotons_fwd");            // Photons vers eau (+z)
    analysisManager->CreateNtupleDColumn("sumEPhotons_fwd_keV");     // Énergie photons (+z)
    analysisManager->CreateNtupleIColumn("nElectrons_fwd");          // Électrons vers eau (+z)
    analysisManager->CreateNtupleDColumn("sumEElectrons_fwd_keV");   // Énergie électrons (+z)
    analysisManager->FinishNtuple();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents == 0) return;
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Remplir le ntuple des raies gamma
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        analysisManager->FillNtupleIColumn(1, 0, i);
        analysisManager->FillNtupleDColumn(1, 1, EventAction::GetGammaLineEnergy(i));
        analysisManager->FillNtupleIColumn(1, 2, fLineEmitted[i]);
        analysisManager->FillNtupleIColumn(1, 3, fLineExitedFilter[i]);
        analysisManager->FillNtupleIColumn(1, 4, fLineAbsorbedFilter[i]);
        analysisManager->FillNtupleIColumn(1, 5, fLineEnteredWater[i]);
        analysisManager->FillNtupleIColumn(1, 6, fLineAbsorbedWater[i]);
        analysisManager->FillNtupleDColumn(1, 7, GetLineFilterAbsorptionRate(i));
        analysisManager->FillNtupleDColumn(1, 8, GetLineWaterAbsorptionRate(i));
        analysisManager->AddNtupleRow(1);
    }
    
    analysisManager->Write();
    analysisManager->CloseFile();
    
    // Affichage des résultats (console + log)
    PrintResults(nEvents);
    PrintDoseTable();
    PrintAbsorptionTable();
    
    Logger::GetInstance()->LogHeader("Fin du Run - Résultats");
    Logger::GetInstance()->Close();
}

// ═══════════════════════════════════════════════════════════════
// AFFICHAGE DES RÉSULTATS
// ═══════════════════════════════════════════════════════════════

void RunAction::PrintResults(G4int nEvents) const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                           RÉSULTATS DU RUN                                    ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  Événements simulés: " << std::setw(10) << nEvents << "                                          ║\n";
    oss << "║  Primaires générés:  " << std::setw(10) << fTotalPrimariesGenerated << "                                          ║\n";
    oss << "║  Transmis:           " << std::setw(10) << fTotalTransmitted << "                                          ║\n";
    oss << "║  Absorbés:           " << std::setw(10) << fTotalAbsorbed << "                                          ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  COMPTEURS DE PASSAGE (plans cylindriques) :                                  ║\n";
    oss << "║    Pre-filtre:       " << std::setw(10) << fGammasPreFilterPlane << "                                          ║\n";
    oss << "║    Post-filtre:      " << std::setw(10) << fGammasPostFilterPlane << "                                          ║\n";
    oss << "║    Pre-container:    " << std::setw(10) << fGammasPreContainerPlane << "  (avant eau, air)                       ║\n";
    oss << "║    Post-container:   " << std::setw(10) << fGammasPostContainerPlane << "  (après eau, W_PETG)                    ║\n";
    oss << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

void RunAction::PrintDoseTable() const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                                  DOSES PAR ANNEAU (nGy)                                       ║\n";
    oss << "╠══════════╦═══════════════╦═══════════════╦═══════════════╦═══════════════╦════════════════════╣\n";
    oss << "║  Anneau  ║   Masse (g)   ║ E_dep (MeV)   ║  Dose (nGy)   ║ Dose/evt(nGy) ║  Événements        ║\n";
    oss << "╠══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    
    G4double totalDose_nGy = 0.;  // CORRIGÉ: somme des doses par anneau
    G4double totalMass = 0.;
    G4int totalEvents = 0;
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double mass_g = fRingMasses[i];
        G4double energy_MeV = fRingTotalEnergy[i] / MeV;
        G4double dose_nGy = EnergyToNanoGray(energy_MeV, mass_g);
        G4double dosePerEvt = (fRingEventCount[i] > 0) ? dose_nGy / fRingEventCount[i] : 0.;
        
        oss << "║    " << std::setw(2) << i << "    ║"
            << std::setw(13) << std::fixed << std::setprecision(4) << mass_g << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << energy_MeV << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << dose_nGy << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << dosePerEvt << "  ║"
            << std::setw(18) << std::fixed << std::setprecision(0) << fRingEventCount[i] << "  ║\n";
        
        totalDose_nGy += dose_nGy;  // CORRIGÉ: somme des doses
        totalMass += mass_g;
        totalEvents = std::max(totalEvents, fRingEventCount[i]);
    }
    
    G4double totalDosePerEvt = (fTotalEvents > 0) ? totalDose_nGy / fTotalEvents : 0.;
    
    oss << "╠══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    oss << "║  TOTAL   ║"
        << std::setw(13) << std::fixed << std::setprecision(4) << totalMass << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << fTotalWaterEnergy/MeV << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << totalDose_nGy << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << totalDosePerEvt << "  ║"
        << std::setw(18) << std::fixed << std::setprecision(0) << fTotalEvents << "  ║\n";
    oss << "╚══════════╩═══════════════╩═══════════════╩═══════════════╩═══════════════╩════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

void RunAction::PrintAbsorptionTable() const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                                    TAUX D'ABSORPTION PAR RAIE GAMMA Eu-152                                         ║\n";
    oss << "╠═══════════════╦═══════════╦═══════════════╦═══════════════╦═══════════════╦═══════════════╦═══════════════════════════╣\n";
    oss << "║     Raie      ║   Émis    ║ Sortis filtre ║  Abs. filtre  ║ Entrés eau    ║   Abs. eau    ║  Taux abs. filtre/eau (%) ║\n";
    oss << "╠═══════════════╬═══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════════════════╣\n";
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4double filterRate = GetLineFilterAbsorptionRate(i);
        G4double waterRate = GetLineWaterAbsorptionRate(i);
        
        oss << "║ " << std::setw(13) << std::left << EventAction::GetGammaLineName(i) << " ║"
            << std::setw(10) << std::right << fLineEmitted[i] << " ║"
            << std::setw(14) << fLineExitedFilter[i] << " ║"
            << std::setw(14) << fLineAbsorbedFilter[i] << " ║"
            << std::setw(14) << fLineEnteredWater[i] << " ║"
            << std::setw(14) << fLineAbsorbedWater[i] << " ║"
            << std::setw(11) << std::fixed << std::setprecision(2) << filterRate << " / "
            << std::setw(10) << std::fixed << std::setprecision(2) << waterRate << " ║\n";
    }
    
    G4int totalEmitted = 0, totalExited = 0, totalAbsFilter = 0;
    G4int totalEntered = 0, totalAbsWater = 0;
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        totalEmitted += fLineEmitted[i];
        totalExited += fLineExitedFilter[i];
        totalAbsFilter += fLineAbsorbedFilter[i];
        totalEntered += fLineEnteredWater[i];
        totalAbsWater += fLineAbsorbedWater[i];
    }
    
    G4double totalFilterRate = (totalEmitted > 0) ? 100. * totalAbsFilter / totalEmitted : 0.;
    G4double totalWaterRate = (totalEntered > 0) ? 100. * totalAbsWater / totalEntered : 0.;
    
    oss << "╠═══════════════╬═══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════════════════╣\n";
    oss << "║     TOTAL     ║"
        << std::setw(10) << totalEmitted << " ║"
        << std::setw(14) << totalExited << " ║"
        << std::setw(14) << totalAbsFilter << " ║"
        << std::setw(14) << totalEntered << " ║"
        << std::setw(14) << totalAbsWater << " ║"
        << std::setw(11) << std::fixed << std::setprecision(2) << totalFilterRate << " / "
        << std::setw(10) << std::fixed << std::setprecision(2) << totalWaterRate << " ║\n";
    oss << "╚═══════════════╩═══════════════╩═══════════════╩═══════════════╩═══════════════╩═══════════════╩═══════════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

// ═══════════════════════════════════════════════════════════════
// ACCUMULATION DES DONNÉES
// ═══════════════════════════════════════════════════════════════

void RunAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingTotalEnergy[ringIndex] += edep;
        fRingTotalEnergy2[ringIndex] += edep * edep;
        fRingEventCount[ringIndex]++;
        
        fTotalWaterEnergy += edep;
        fTotalWaterEventCount++;
        
        // Remplir l'histogramme de dose par anneau
        auto analysisManager = G4AnalysisManager::Instance();
        G4double dose_nGy = EnergyToNanoGray(edep / MeV, fRingMasses[ringIndex]);
        analysisManager->FillH1(ringIndex, dose_nGy);
    }
}

void RunAction::AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings &&
        lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        fRingEnergyByLine[ringIndex][lineIndex] += edep;
        
        auto analysisManager = G4AnalysisManager::Instance();
        analysisManager->FillH1(6 + lineIndex, edep / keV);
    }
}

void RunAction::RecordEventStatistics(G4int nPrimaries, 
                                      const std::vector<G4double>& primaryEnergies,
                                      G4int nTransmitted,
                                      G4int nAbsorbed,
                                      G4double totalDeposit,
                                      const std::array<G4double, 5>& ringEnergies)
{
    fTotalEvents++;
    fTotalPrimariesGenerated += nPrimaries;
    fTotalTransmitted += nTransmitted;
    fTotalAbsorbed += nAbsorbed;
    
    if (nPrimaries == 0) {
        fTotalEventsWithZeroGamma++;
    }
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    // ═══════════════════════════════════════════════════════════════
    // CORRECTION: Calculer la dose totale comme SOMME des doses par anneau
    // (et non pas comme totalDeposit / totalMass)
    // ═══════════════════════════════════════════════════════════════
    
    G4double totalDose_nGy = 0.;
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double dose_nGy = EnergyToNanoGray(ringEnergies[i] / MeV, fRingMasses[i]);
        analysisManager->FillNtupleDColumn(0, i, dose_nGy);
        totalDose_nGy += dose_nGy;  // Somme des doses par anneau
    }
    
    // CORRIGÉ: Utiliser totalDose_nGy (somme des doses) au lieu de eventDose_nGy
    analysisManager->FillNtupleDColumn(0, DetectorConstruction::kNbWaterRings, totalDose_nGy);
    analysisManager->FillNtupleDColumn(0, DetectorConstruction::kNbWaterRings + 1, totalDeposit / keV);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 2, nPrimaries);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 3, nTransmitted);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 4, nAbsorbed);
    analysisManager->AddNtupleRow(0);
    
    // CORRIGÉ: Remplir l'histogramme de dose totale avec la somme des doses
    if (totalDeposit > 0.) {
        analysisManager->FillH1(5, totalDose_nGy);
    }
}

void RunAction::RecordContainerPlaneStatistics(
    G4int preNPhotons, G4double preSumEPhotons,
    G4int preNElectrons, G4double preSumEElectrons,
    G4int postNPhotonsBack, G4double postSumEPhotonsBack,
    G4int postNElectronsBack, G4double postSumEElectronsBack,
    G4int postNPhotonsFwd, G4double postSumEPhotonsFwd,
    G4int postNElectronsFwd, G4double postSumEElectronsFwd)
{
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Ntuple 2 : precontainer
    analysisManager->FillNtupleIColumn(2, 0, preNPhotons);
    analysisManager->FillNtupleDColumn(2, 1, preSumEPhotons / keV);
    analysisManager->FillNtupleIColumn(2, 2, preNElectrons);
    analysisManager->FillNtupleDColumn(2, 3, preSumEElectrons / keV);
    analysisManager->AddNtupleRow(2);
    
    // Ntuple 3 : postcontainer
    analysisManager->FillNtupleIColumn(3, 0, postNPhotonsBack);
    analysisManager->FillNtupleDColumn(3, 1, postSumEPhotonsBack / keV);
    analysisManager->FillNtupleIColumn(3, 2, postNElectronsBack);
    analysisManager->FillNtupleDColumn(3, 3, postSumEElectronsBack / keV);
    analysisManager->FillNtupleIColumn(3, 4, postNPhotonsFwd);
    analysisManager->FillNtupleDColumn(3, 5, postSumEPhotonsFwd / keV);
    analysisManager->FillNtupleIColumn(3, 6, postNElectronsFwd);
    analysisManager->FillNtupleDColumn(3, 7, postSumEElectronsFwd / keV);
    analysisManager->AddNtupleRow(3);
}

void RunAction::RecordGammaLineStatistics(G4int lineIndex,
                                          G4bool exitedFilter,
                                          G4bool absorbedInFilter,
                                          G4bool enteredWater,
                                          G4bool absorbedInWater)
{
    if (lineIndex < 0 || lineIndex >= EventAction::kNbGammaLines) return;
    
    fLineEmitted[lineIndex]++;
    
    if (exitedFilter) fLineExitedFilter[lineIndex]++;
    if (absorbedInFilter) fLineAbsorbedFilter[lineIndex]++;
    if (enteredWater) fLineEnteredWater[lineIndex]++;
    if (absorbedInWater) fLineAbsorbedWater[lineIndex]++;
}

// ═══════════════════════════════════════════════════════════════
// ACCESSEURS POUR LES STATISTIQUES PAR RAIE
// ═══════════════════════════════════════════════════════════════

G4int RunAction::GetLineEmitted(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        return fLineEmitted[lineIndex];
    }
    return 0;
}

G4int RunAction::GetLineAbsorbedFilter(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        return fLineAbsorbedFilter[lineIndex];
    }
    return 0;
}

G4int RunAction::GetLineAbsorbedWater(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        return fLineAbsorbedWater[lineIndex];
    }
    return 0;
}

G4double RunAction::GetLineFilterAbsorptionRate(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        if (fLineEmitted[lineIndex] > 0) {
            return static_cast<G4double>(fLineAbsorbedFilter[lineIndex]) / 
                   static_cast<G4double>(fLineEmitted[lineIndex]) * 100.;
        }
    }
    return 0.;
}

G4double RunAction::GetLineWaterAbsorptionRate(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        if (fLineEnteredWater[lineIndex] > 0) {
            return static_cast<G4double>(fLineAbsorbedWater[lineIndex]) / 
                   static_cast<G4double>(fLineEnteredWater[lineIndex]) * 100.;
        }
    }
    return 0.;
}

// ═══════════════════════════════════════════════════════════════
// CALCULS DE NORMALISATION
// ═══════════════════════════════════════════════════════════════

G4double RunAction::CalculateIrradiationTime(G4int nEvents) const
{
    G4double activityInCone = fActivity4pi * GetSolidAngleFraction();
    if (activityInCone > 0.) {
        return static_cast<G4double>(nEvents) / activityInCone;
    }
    return 0.;
}

G4double RunAction::CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const
{
    G4double time = CalculateIrradiationTime(nEvents);
    if (time > 0.) {
        return totalDose_Gy / time;
    }
    return 0.;
}
