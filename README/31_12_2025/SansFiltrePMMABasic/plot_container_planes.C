// ═══════════════════════════════════════════════════════════════════════════
// Script ROOT : Histogrammes des plans PreContainer et PostContainer
// Usage : root -l plot_container_planes.C
// ═══════════════════════════════════════════════════════════════════════════

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TPaveStats.h>
#include <iostream>

// ═══════════════════════════════════════════════════════════════════════════
// Fonction pour configurer le pave de stats d'un histogramme
// ═══════════════════════════════════════════════════════════════════════════
void ConfigureStatsPave(TH1D* hist, 
                        Double_t x1 = 0.65, Double_t y1 = 0.65, 
                        Double_t x2 = 0.95, Double_t y2 = 0.90,
                        Double_t textSize = 0.04) {
    gPad->Update();
    TPaveStats* stats = (TPaveStats*)hist->FindObject("stats");
    if (stats) {
        stats->SetX1NDC(x1);
        stats->SetY1NDC(y1);
        stats->SetX2NDC(x2);
        stats->SetY2NDC(y2);
        stats->SetTextSize(textSize);
        stats->SetTextFont(42);  // Helvetica
        stats->SetFillColor(kWhite);
        stats->SetFillStyle(1001);
        stats->SetBorderSize(1);
        gPad->Modified();
    }
}

void plot_container_planes(const char* filename = "puits_couronne.root") {
    
    // ═══════════════════════════════════════════════════════════════════════
    // Configuration du style global
    // ═══════════════════════════════════════════════════════════════════════
    
    gStyle->SetOptStat("emr");  // entries, mean, rms (sans underflow/overflow)
    gStyle->SetOptFit(0);
    gStyle->SetHistLineWidth(2);
    gStyle->SetTitleSize(0.05, "XYZ");
    gStyle->SetLabelSize(0.04, "XYZ");
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.05);
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetStatBorderSize(1);
    gStyle->SetStatFont(42);
    gStyle->SetStatFontSize(0.035);
    
    // Titre des histogrammes
    gStyle->SetTitleSize(0.06, "");     // Taille du titre principal
    gStyle->SetTitleFont(62, "");       // Police en gras (62 = Helvetica Bold)

    // Titres des axes
    gStyle->SetTitleSize(0.055, "XYZ"); // Taille des titres d'axes
    gStyle->SetTitleFont(62, "XYZ");    // Police en gras pour les titres d'axes

    // Labels des échelles (nombres sur les axes)
    gStyle->SetLabelSize(0.05, "XYZ");  // Taille des labels
    gStyle->SetLabelFont(62, "XYZ");    // Police en gras pour les labels
    
    // ═══════════════════════════════════════════════════════════════════════
    // Ouverture du fichier ROOT
    // ═══════════════════════════════════════════════════════════════════════
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
        return;
    }
    
    std::cout << "\n=== Fichier ouvert: " << filename << " ===" << std::endl;
    
    // Récupérer les TTrees
    TTree* treePreContainer = (TTree*)file->Get("precontainer");
    TTree* treePostContainer = (TTree*)file->Get("postcontainer");
    
    if (!treePreContainer) {
        std::cerr << "Erreur: ntuple 'precontainer' non trouvé!" << std::endl;
        return;
    }
    if (!treePostContainer) {
        std::cerr << "Erreur: ntuple 'postcontainer' non trouvé!" << std::endl;
        return;
    }
    
    Long64_t nEntriesPre = treePreContainer->GetEntries();
    Long64_t nEntriesPost = treePostContainer->GetEntries();
    
    std::cout << "Ntuple precontainer: " << nEntriesPre << " événements" << std::endl;
    std::cout << "Ntuple postcontainer: " << nEntriesPost << " événements" << std::endl;
    
    // Position par défaut du pave de stats
    Double_t statX1 = 0.65, statX2 = 0.95;
    Double_t statY1 = 0.70, statY2 = 0.90;
    Double_t statTextSize = 0.05;

    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 1 : Plan PreContainer (4 histogrammes)
    // Particules ENTRANT dans l'eau (direction +z)
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c1 = new TCanvas("c1", "Plan PreContainerPlane (avant eau, GAP=0, Air)", 1400, 1000);
    c1->Divide(2, 2);
    
    // --- Histogramme 1.1 : Nombre de photons entrant dans l'eau ---
    c1->cd(1);
    gPad->SetLogy();
    TH1D* h_pre_nPhotons = new TH1D("h_pre_nPhotons", 
        "Nombre de photons entrant dans l'eau par d#acute{e}sint#acute{e}gration;N_{#gamma} entrant (+z);Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    h_pre_nPhotons->SetLineColor(kOrange+1);
    h_pre_nPhotons->SetFillColor(kOrange-9);
    h_pre_nPhotons->SetFillStyle(3001);
    treePreContainer->Draw("nPhotons>>h_pre_nPhotons", "", "");
    ConfigureStatsPave(h_pre_nPhotons, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 1.2 : Somme énergie photons entrant ---
    c1->cd(2);
    gPad->SetLogy();
    TH1D* h_pre_sumEPhotons = new TH1D("h_pre_sumEPhotons", 
        "Somme des #acute{e}nergies des photons entrant;#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 10000);
    h_pre_sumEPhotons->SetLineColor(kOrange+1);
    h_pre_sumEPhotons->SetFillColor(kOrange-9);
    h_pre_sumEPhotons->SetFillStyle(3001);
    treePreContainer->Draw("sumEPhotons_keV>>h_pre_sumEPhotons", "sumEPhotons_keV>0", "");
    ConfigureStatsPave(h_pre_sumEPhotons, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 1.3 : Nombre d'électrons entrant dans l'eau ---
    c1->cd(3);
    gPad->SetLogy();
    TH1D* h_pre_nElectrons = new TH1D("h_pre_nElectrons", 
        "Nombre d'#acute{e}lectrons entrant dans l'eau par d#acute{e}sint#acute{e}gration;N_{e^{-}} entrant (+z);Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    h_pre_nElectrons->SetLineColor(kGreen+2);
    h_pre_nElectrons->SetFillColor(kGreen-9);
    h_pre_nElectrons->SetFillStyle(3001);
    treePreContainer->Draw("nElectrons>>h_pre_nElectrons", "", "");
    ConfigureStatsPave(h_pre_nElectrons, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 1.4 : Somme énergie électrons entrant ---
    c1->cd(4);
    gPad->SetLogy();
    TH1D* h_pre_sumEElectrons = new TH1D("h_pre_sumEElectrons", 
        "Somme des #acute{e}nergies des #acute{e}lectrons entrant;#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 2000);
    h_pre_sumEElectrons->SetLineColor(kGreen+2);
    h_pre_sumEElectrons->SetFillColor(kGreen-9);
    h_pre_sumEElectrons->SetFillStyle(3001);
    treePreContainer->Draw("sumEElectrons_keV>>h_pre_sumEElectrons", "sumEElectrons_keV>0", "");
    ConfigureStatsPave(h_pre_sumEElectrons, statX1, statY1, statX2, statY2, statTextSize);
    
    c1->Update();
    c1->SaveAs("histos_precontainer.png");
    
    std::cout << "\n>>> Canvas 1 sauvegardé: histos_precontainer.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 2 : Plan PostContainer - Photons TRANSMIS (sortant de l'eau, +z)
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c2 = new TCanvas("c2", "Plan PostContainerPlane - Photons transmis (sortant, +z)", 1400, 500);
    c2->Divide(2, 1);
    
    // --- Histogramme 2.1 : Nombre de photons transmis (sortant) ---
    c2->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nPhotons_fwd = new TH1D("h_post_nPhotons_fwd", 
        "Nombre de photons transmis (sortant de l'eau);N_{#gamma} transmis (+z);Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    h_post_nPhotons_fwd->SetLineColor(kCyan+2);
    h_post_nPhotons_fwd->SetFillColor(kCyan-9);
    h_post_nPhotons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("nPhotons_fwd>>h_post_nPhotons_fwd", "", "");
    ConfigureStatsPave(h_post_nPhotons_fwd, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 2.2 : Somme énergie photons transmis ---
    c2->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEPhotons_fwd = new TH1D("h_post_sumEPhotons_fwd", 
        "Somme des #acute{e}nergies des photons transmis;#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 10000);
    h_post_sumEPhotons_fwd->SetLineColor(kCyan+2);
    h_post_sumEPhotons_fwd->SetFillColor(kCyan-9);
    h_post_sumEPhotons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("sumEPhotons_fwd_keV>>h_post_sumEPhotons_fwd", "sumEPhotons_fwd_keV>0", "");
    ConfigureStatsPave(h_post_sumEPhotons_fwd, statX1, statY1, statX2, statY2, statTextSize);
    
    c2->Update();
    c2->SaveAs("histos_postcontainer_photons_transmis.png");
    
    std::cout << ">>> Canvas 2 sauvegardé: histos_postcontainer_photons_transmis.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 3 : Plan PostContainer - Photons RÉTRODIFFUSÉS (retournant vers l'eau, -z)
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c3 = new TCanvas("c3", "Plan PostContainerPlane - Photons rétrodiffusés (-z)", 1400, 500);
    c3->Divide(2, 1);
    
    // --- Histogramme 3.1 : Nombre de photons rétrodiffusés ---
    c3->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nPhotons_back = new TH1D("h_post_nPhotons_back", 
        "Nombre de photons r#acute{e}trodiffus#acute{e}s (retournant vers l'eau);N_{#gamma} backscatter (-z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nPhotons_back->SetLineColor(kViolet+1);
    h_post_nPhotons_back->SetFillColor(kViolet-9);
    h_post_nPhotons_back->SetFillStyle(3001);
    treePostContainer->Draw("nPhotons_back>>h_post_nPhotons_back", "", "");
    ConfigureStatsPave(h_post_nPhotons_back, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 3.2 : Somme énergie photons rétrodiffusés ---
    c3->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEPhotons_back = new TH1D("h_post_sumEPhotons_back", 
        "Somme des #acute{e}nergies des photons r#acute{e}trodiffus#acute{e}s;#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1500);
    h_post_sumEPhotons_back->SetLineColor(kViolet+1);
    h_post_sumEPhotons_back->SetFillColor(kViolet-9);
    h_post_sumEPhotons_back->SetFillStyle(3001);
    treePostContainer->Draw("sumEPhotons_back_keV>>h_post_sumEPhotons_back", "sumEPhotons_back_keV>0", "");
    ConfigureStatsPave(h_post_sumEPhotons_back, statX1, statY1, statX2, statY2, statTextSize);
    
    c3->Update();
    c3->SaveAs("histos_postcontainer_photons_backscatter.png");
    
    std::cout << ">>> Canvas 3 sauvegardé: histos_postcontainer_photons_backscatter.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 4 : Comparaison PreContainer vs PostContainer (photons)
    // Entrant vs Transmis
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c4 = new TCanvas("c4", "Comparaison Photons: Entrant (Pre) vs Transmis (Post)", 1400, 500);
    c4->Divide(2, 1);
    
    // --- Comparaison nombre de photons ---
    c4->cd(1);
    gPad->SetLogy();
    
    TH1D* h_comp_pre_n = new TH1D("h_comp_pre_n", 
        "Comparaison N_{#gamma} : Entrant vs Transmis;N_{#gamma};Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    TH1D* h_comp_post_n = new TH1D("h_comp_post_n", "", 20, 0, 20);
    
    h_comp_pre_n->SetLineColor(kOrange+1);
    h_comp_pre_n->SetLineWidth(2);
    h_comp_post_n->SetLineColor(kCyan+2);
    h_comp_post_n->SetLineWidth(2);
    
    treePreContainer->Draw("nPhotons>>h_comp_pre_n", "", "");
    ConfigureStatsPave(h_comp_pre_n, 0.65, 0.55, 0.95, 0.70, statTextSize);
    
    treePostContainer->Draw("nPhotons_fwd>>h_comp_post_n", "", "sames");
    ConfigureStatsPave(h_comp_post_n, 0.65, 0.35, 0.95, 0.50, statTextSize);
    
    TLegend* leg1 = new TLegend(0.35, 0.82, 0.8, 0.92);
    leg1->AddEntry(h_comp_pre_n, "PreContainer (entrant dans l'eau)", "l");
    leg1->AddEntry(h_comp_post_n, "PostContainer (transmis, sortant)", "l");
    leg1->SetTextSize(0.035);
    leg1->Draw();
    
    // --- Comparaison énergie photons ---
    c4->cd(2);
    gPad->SetLogy();
    
    TH1D* h_comp_pre_e = new TH1D("h_comp_pre_e", 
        "Comparaison #SigmaE_{#gamma} : Entrant vs Transmis;#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 10000);
    TH1D* h_comp_post_e = new TH1D("h_comp_post_e", "", 100, 0, 10000);
    
    h_comp_pre_e->SetLineColor(kOrange+1);
    h_comp_pre_e->SetLineWidth(2);
    h_comp_post_e->SetLineColor(kCyan+2);
    h_comp_post_e->SetLineWidth(2);
    
    treePreContainer->Draw("sumEPhotons_keV>>h_comp_pre_e", "sumEPhotons_keV>0", "");
    ConfigureStatsPave(h_comp_pre_e, 0.65, 0.55, 0.95, 0.70, statTextSize);
    
    treePostContainer->Draw("sumEPhotons_fwd_keV>>h_comp_post_e", "sumEPhotons_fwd_keV>0", "sames");
    ConfigureStatsPave(h_comp_post_e, 0.65, 0.35, 0.95, 0.50, statTextSize);
    
    TLegend* leg2 = new TLegend(0.35, 0.82, 0.8, 0.92);
    leg2->AddEntry(h_comp_pre_e, "PreContainer (entrant dans l'eau)", "l");
    leg2->AddEntry(h_comp_post_e, "PostContainer (transmis, sortant)", "l");
    leg2->SetTextSize(0.035);
    leg2->Draw();
    
    c4->Update();
    c4->SaveAs("histos_comparison_photons.png");
    
    std::cout << ">>> Canvas 4 sauvegardé: histos_comparison_photons.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 5 : Plan PostContainer - Électrons TRANSMIS (sortant de l'eau, +z)
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c5 = new TCanvas("c5", "Plan PostContainerPlane - Electrons transmis (sortant, +z)", 1400, 500);
    c5->Divide(2, 1);
    
    // --- Histogramme 5.1 : Nombre d'électrons transmis (sortant) ---
    c5->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nElectrons_fwd = new TH1D("h_post_nElectrons_fwd", 
        "Nombre d'#acute{e}lectrons transmis (sortant de l'eau);N_{e^{-}} transmis (+z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nElectrons_fwd->SetLineColor(kBlue+1);
    h_post_nElectrons_fwd->SetFillColor(kBlue-9);
    h_post_nElectrons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("nElectrons_fwd>>h_post_nElectrons_fwd", "", "");
    ConfigureStatsPave(h_post_nElectrons_fwd, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 5.2 : Somme énergie électrons transmis ---
    c5->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEElectrons_fwd = new TH1D("h_post_sumEElectrons_fwd", 
        "Somme des #acute{e}nergies des #acute{e}lectrons transmis;#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1000);
    h_post_sumEElectrons_fwd->SetLineColor(kBlue+1);
    h_post_sumEElectrons_fwd->SetFillColor(kBlue-9);
    h_post_sumEElectrons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("sumEElectrons_fwd_keV>>h_post_sumEElectrons_fwd", "sumEElectrons_fwd_keV>0", "");
    ConfigureStatsPave(h_post_sumEElectrons_fwd, statX1, statY1, statX2, statY2, statTextSize);
    
    c5->Update();
    c5->SaveAs("histos_postcontainer_electrons_transmis.png");
    
    std::cout << ">>> Canvas 5 sauvegardé: histos_postcontainer_electrons_transmis.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 6 : Plan PostContainer - Électrons RÉTRODIFFUSÉS (retournant vers l'eau, -z)
    // ═══════════════════════════════════════════════════════════════════════

    TCanvas* c6 = new TCanvas("c6", "Plan PostContainerPlane - Electrons rétrodiffusés (-z)", 1400, 500);
    c6->Divide(2, 1);
    
    // --- Histogramme 6.1 : Nombre d'électrons rétrodiffusés ---
    c6->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nElectrons_back = new TH1D("h_post_nElectrons_back", 
        "Nombre d'#acute{e}lectrons r#acute{e}trodiffus#acute{e}s (retournant vers l'eau);N_{e^{-}} backscatter (-z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nElectrons_back->SetLineColor(kRed+1);
    h_post_nElectrons_back->SetFillColor(kRed-9);
    h_post_nElectrons_back->SetFillStyle(3001);
    treePostContainer->Draw("nElectrons_back>>h_post_nElectrons_back", "", "");
    ConfigureStatsPave(h_post_nElectrons_back, statX1, statY1, statX2, statY2, statTextSize);
    
    // --- Histogramme 6.2 : Somme énergie électrons rétrodiffusés ---
    c6->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEElectrons_back = new TH1D("h_post_sumEElectrons_back", 
        "Somme des #acute{e}nergies des #acute{e}lectrons r#acute{e}trodiffus#acute{e}s;#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1000);
    h_post_sumEElectrons_back->SetLineColor(kRed+1);
    h_post_sumEElectrons_back->SetFillColor(kRed-9);
    h_post_sumEElectrons_back->SetFillStyle(3001);
    treePostContainer->Draw("sumEElectrons_back_keV>>h_post_sumEElectrons_back", "sumEElectrons_back_keV>0", "");
    ConfigureStatsPave(h_post_sumEElectrons_back, statX1, statY1, statX2, statY2, statTextSize);
    
    c6->Update();
    c6->SaveAs("histos_postcontainer_electrons_backscatter.png");
    
    std::cout << ">>> Canvas 6 sauvegardé: histos_postcontainer_electrons_backscatter.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Affichage des statistiques
    // ═══════════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    STATISTIQUES DES HISTOGRAMMES                ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ PRECONTAINER (particules ENTRANT dans l'eau)                   ║" << std::endl;
    std::cout << "║   Photons entrant:     Mean = " << h_pre_nPhotons->GetMean() 
              << "  RMS = " << h_pre_nPhotons->GetRMS() << std::endl;
    std::cout << "║   Energie photons:     Mean = " << h_pre_sumEPhotons->GetMean() 
              << " keV" << std::endl;
    std::cout << "║   Electrons entrant:   Mean = " << h_pre_nElectrons->GetMean() 
              << "  RMS = " << h_pre_nElectrons->GetRMS() << std::endl;
    std::cout << "║   Energie electrons:   Mean = " << h_pre_sumEElectrons->GetMean() 
              << " keV" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ POSTCONTAINER - Particules TRANSMISES (sortant de l'eau, +z)   ║" << std::endl;
    std::cout << "║   Photons transmis:    Mean = " << h_post_nPhotons_fwd->GetMean() 
              << "  RMS = " << h_post_nPhotons_fwd->GetRMS() << std::endl;
    std::cout << "║   Energie photons:     Mean = " << h_post_sumEPhotons_fwd->GetMean() 
              << " keV" << std::endl;
    std::cout << "║   Electrons transmis:  Mean = " << h_post_nElectrons_fwd->GetMean() 
              << "  RMS = " << h_post_nElectrons_fwd->GetRMS() << std::endl;
    std::cout << "║   Energie electrons:   Mean = " << h_post_sumEElectrons_fwd->GetMean() 
              << " keV" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ POSTCONTAINER - Particules RÉTRODIFFUSÉES (vers l'eau, -z)     ║" << std::endl;
    std::cout << "║   Photons backscatter: Mean = " << h_post_nPhotons_back->GetMean() 
              << "  RMS = " << h_post_nPhotons_back->GetRMS() << std::endl;
    std::cout << "║   Energie photons:     Mean = " << h_post_sumEPhotons_back->GetMean() 
              << " keV" << std::endl;
    std::cout << "║   Electrons backscatter: Mean = " << h_post_nElectrons_back->GetMean() 
              << "  RMS = " << h_post_nElectrons_back->GetRMS() << std::endl;
    std::cout << "║   Energie electrons:   Mean = " << h_post_sumEElectrons_back->GetMean() 
              << " keV" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\n=== Script terminé avec succès ===" << std::endl;
    std::cout << "Fichiers générés:" << std::endl;
    std::cout << "  - histos_precontainer.png              (photons/electrons entrant)" << std::endl;
    std::cout << "  - histos_postcontainer_photons_transmis.png    (photons sortant +z)" << std::endl;
    std::cout << "  - histos_postcontainer_photons_backscatter.png (photons backscatter -z)" << std::endl;
    std::cout << "  - histos_comparison_photons.png        (entrant vs transmis)" << std::endl;
    std::cout << "  - histos_postcontainer_electrons_transmis.png  (electrons sortant +z)" << std::endl;
    std::cout << "  - histos_postcontainer_electrons_backscatter.png (electrons backscatter -z)" << std::endl;
}
