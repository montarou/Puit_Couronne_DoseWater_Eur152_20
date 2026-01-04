// ═══════════════════════════════════════════════════════════════════════════════
// compare_dose_anneau_central.C
// Script ROOT pour comparer la dose dans l'anneau central (anneau 0)
// entre deux configurations : avec et sans PMMA/W
//
// Usage: root -l 'compare_dose_anneau_central.C("puits_couronne_Avec_PMMA_W.root", "puits_couronne_Sans_PMMA_W.root")'
// ═══════════════════════════════════════════════════════════════════════════════

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <iostream>
#include <iomanip>

void compare_dose_anneau_central(
    const char* file_avec_PMMA_W = "puits_couronne_Avec_PMMA_W.root",
    const char* file_sans_PMMA_W = "puits_couronne_Sans_PMMA_W.root"
) {
    // ═══════════════════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════════
    
    gStyle->SetOptStat(0);  // Pas de boîte de stats par défaut
    gStyle->SetTitleFontSize(0.04);
    gStyle->SetLabelSize(0.035, "XY");
    gStyle->SetTitleSize(0.04, "XY");
    
    // Paramètres des histogrammes
    int nBins = 100;
    double xMin = 0.0;
    double xMax = 0.5;  // Ajuster si nécessaire
    
    // ═══════════════════════════════════════════════════════════════════════════
    // OUVERTURE DES FICHIERS
    // ═══════════════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     COMPARAISON DOSE ANNEAU CENTRAL - AVEC/SANS PMMA+W           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Fichier AVEC PMMA + W
    TFile* f_avec = TFile::Open(file_avec_PMMA_W);
    if (!f_avec || f_avec->IsZombie()) {
        std::cerr << "ERREUR: Impossible d'ouvrir " << file_avec_PMMA_W << std::endl;
        return;
    }
    std::cout << "✓ Fichier ouvert: " << file_avec_PMMA_W << std::endl;
    
    // Fichier SANS PMMA + W
    TFile* f_sans = TFile::Open(file_sans_PMMA_W);
    if (!f_sans || f_sans->IsZombie()) {
        std::cerr << "ERREUR: Impossible d'ouvrir " << file_sans_PMMA_W << std::endl;
        return;
    }
    std::cout << "✓ Fichier ouvert: " << file_sans_PMMA_W << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // RÉCUPÉRATION DES TREES
    // ═══════════════════════════════════════════════════════════════════════════
    
    TTree* tree_avec = (TTree*)f_avec->Get("doses");
    if (!tree_avec) {
        std::cerr << "ERREUR: Tree 'doses' non trouvé dans " << file_avec_PMMA_W << std::endl;
        return;
    }
    
    TTree* tree_sans = (TTree*)f_sans->Get("doses");
    if (!tree_sans) {
        std::cerr << "ERREUR: Tree 'doses' non trouvé dans " << file_sans_PMMA_W << std::endl;
        return;
    }
    
    Long64_t nEvents_avec = tree_avec->GetEntries();
    Long64_t nEvents_sans = tree_sans->GetEntries();
    
    std::cout << "\nNombre d'événements:" << std::endl;
    std::cout << "  - Avec PMMA+W:  " << nEvents_avec << std::endl;
    std::cout << "  - Sans PMMA+W:  " << nEvents_sans << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CRÉATION DES HISTOGRAMMES
    // ═══════════════════════════════════════════════════════════════════════════
    
    TH1D* h_avec = new TH1D("h_avec", "Dose anneau 0 - Avec PMMA+W", nBins, xMin, xMax);
    TH1D* h_sans = new TH1D("h_sans", "Dose anneau 0 - Sans PMMA+W", nBins, xMin, xMax);
    
    h_avec->SetLineColor(kBlue+1);
    h_avec->SetLineWidth(2);
    h_avec->SetFillColor(kBlue-9);
    h_avec->SetFillStyle(3004);
    
    h_sans->SetLineColor(kRed+1);
    h_sans->SetLineWidth(2);
    h_sans->SetFillColor(kRed-9);
    h_sans->SetFillStyle(3005);
    
    // ═══════════════════════════════════════════════════════════════════════════
    // REMPLISSAGE DES HISTOGRAMMES
    // ═══════════════════════════════════════════════════════════════════════════
    
    // Variables pour lecture
    double dose_ring0_avec, dose_ring0_sans;
    
    // Configuration des branches - AVEC PMMA+W
    tree_avec->SetBranchAddress("dose_nGy_ring0", &dose_ring0_avec);
    
    // Configuration des branches - SANS PMMA+W
    tree_sans->SetBranchAddress("dose_nGy_ring0", &dose_ring0_sans);
    
    std::cout << "\nRemplissage des histogrammes..." << std::endl;
    
    // Remplissage AVEC PMMA+W
    for (Long64_t i = 0; i < nEvents_avec; i++) {
        tree_avec->GetEntry(i);
        if (dose_ring0_avec > 0) {
            h_avec->Fill(dose_ring0_avec);
        }
    }
    
    // Remplissage SANS PMMA+W
    for (Long64_t i = 0; i < nEvents_sans; i++) {
        tree_sans->GetEntry(i);
        if (dose_ring0_sans > 0) {
            h_sans->Fill(dose_ring0_sans);
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CALCUL DES STATISTIQUES
    // ═══════════════════════════════════════════════════════════════════════════
    
    double mean_avec = h_avec->GetMean();
    double rms_avec = h_avec->GetStdDev();
    int entries_avec = h_avec->GetEntries();
    
    double mean_sans = h_sans->GetMean();
    double rms_sans = h_sans->GetStdDev();
    int entries_sans = h_sans->GetEntries();
    
    // Calcul du rapport
    double ratio = (mean_sans > 0) ? mean_avec / mean_sans : 0;
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    STATISTIQUES ANNEAU CENTRAL                    ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ AVEC PMMA + W:                                                    ║" << std::endl;
    std::cout << "║   Événements avec dépôt: " << std::setw(10) << entries_avec << "                             ║" << std::endl;
    std::cout << "║   Dose moyenne:          " << std::scientific << std::setprecision(3) << mean_avec << " nGy                      ║" << std::endl;
    std::cout << "║   Écart-type:            " << std::scientific << std::setprecision(3) << rms_avec << " nGy                      ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ SANS PMMA + W:                                                    ║" << std::endl;
    std::cout << "║   Événements avec dépôt: " << std::setw(10) << entries_sans << "                             ║" << std::endl;
    std::cout << "║   Dose moyenne:          " << std::scientific << std::setprecision(3) << mean_sans << " nGy                      ║" << std::endl;
    std::cout << "║   Écart-type:            " << std::scientific << std::setprecision(3) << rms_sans << " nGy                      ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ RAPPORT (Avec/Sans):     " << std::fixed << std::setprecision(3) << ratio << "                                   ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CRÉATION DU CANVAS ET AFFICHAGE
    // ═══════════════════════════════════════════════════════════════════════════
    
    TCanvas* c1 = new TCanvas("c1", "Comparaison dose anneau central", 1000, 700);
    c1->SetLogy();
    c1->SetLeftMargin(0.12);
    c1->SetRightMargin(0.05);
    c1->SetTopMargin(0.08);
    c1->SetBottomMargin(0.12);
    
    // Déterminer le maximum pour l'échelle Y
    double yMax = 1.2 * std::max(h_avec->GetMaximum(), h_sans->GetMaximum());
    
    // Dessiner le premier histogramme
    h_sans->SetTitle("Comparaison de la dose dans l'anneau central (r = 0-5 mm);Dose par d#acute{e}sint#acute{e}gration (nGy);Nombre d'#acute{e}v#acute{e}nements");
    h_sans->SetMaximum(yMax);
    h_sans->GetYaxis()->SetTitleOffset(1.2);
    h_sans->Draw("HIST");
    
    // Superposer le second histogramme
    h_avec->Draw("HIST SAME");
    
    // ═══════════════════════════════════════════════════════════════════════════
    // LÉGENDE
    // ═══════════════════════════════════════════════════════════════════════════
    
    TLegend* leg = new TLegend(0.50, 0.82, 0.90, 0.92);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.030);
    leg->AddEntry(h_avec, Form("Avec PMMA+W (N=%d)", entries_avec), "lf");
    leg->AddEntry(h_sans, Form("Sans PMMA+W (N=%d)", entries_sans), "lf");
    leg->Draw();
    
    // ═══════════════════════════════════════════════════════════════════════════
    // BOÎTE DE STATISTIQUES
    // ═══════════════════════════════════════════════════════════════════════════
    
    TPaveText* stats = new TPaveText(0.50, 0.62, 0.90, 0.82, "NDC");
    stats->SetBorderSize(1);
    stats->SetFillColor(kWhite);
    stats->SetTextAlign(12);
    stats->SetTextSize(0.028);
    stats->SetTextFont(42);
    
    stats->AddText("#bf{Avec PMMA+W:}");
    stats->AddText(Form("  #LT D #GT = %.3e nGy", mean_avec));
    stats->AddText(Form("  #sigma = %.3e nGy", rms_avec));
    stats->AddText("");
    stats->AddText("#bf{Sans PMMA+W:}");
    stats->AddText(Form("  #LT D #GT = %.3e nGy", mean_sans));
    stats->AddText(Form("  #sigma = %.3e nGy", rms_sans));
    stats->AddText("");
    stats->AddText(Form("#bf{Rapport Avec/Sans = %.2f}", ratio));
    stats->Draw();
    
    // ═══════════════════════════════════════════════════════════════════════════
    // SAUVEGARDE
    // ═══════════════════════════════════════════════════════════════════════════
    
    c1->SaveAs("comparaison_dose_anneau_central.png");
    c1->SaveAs("comparaison_dose_anneau_central.pdf");
    
    std::cout << "\n✓ Figure sauvegardée: comparaison_dose_anneau_central.png" << std::endl;
    std::cout << "✓ Figure sauvegardée: comparaison_dose_anneau_central.pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // NETTOYAGE
    // ═══════════════════════════════════════════════════════════════════════════
    
    // Garder les fichiers ouverts pour l'affichage interactif
    // f_avec->Close();
    // f_sans->Close();
}
