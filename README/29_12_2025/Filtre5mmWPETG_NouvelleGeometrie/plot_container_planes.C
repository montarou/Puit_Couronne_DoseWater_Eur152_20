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
#include <iostream>

void plot_container_planes(const char* filename = "puits_couronne.root") {
    
    // ═══════════════════════════════════════════════════════════════════════
    // Configuration du style
    // ═══════════════════════════════════════════════════════════════════════
    
    gStyle->SetOptStat(1111);
    gStyle->SetOptFit(0);
    gStyle->SetHistLineWidth(2);
    gStyle->SetTitleSize(0.05, "XYZ");
    gStyle->SetLabelSize(0.04, "XYZ");
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.05);
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    
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
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 1 : Plan PreContainer (4 histogrammes)
    // ═══════════════════════════════════════════════════════════════════════
    
    TCanvas* c1 = new TCanvas("c1", "Plan PreContainerPlane (avant eau, GAP=0, Air)", 1400, 1000);
    c1->Divide(2, 2);
    
    // --- Histogramme 1.1 : Nombre de photons ---
    c1->cd(1);
    gPad->SetLogy();
    TH1D* h_pre_nPhotons = new TH1D("h_pre_nPhotons", 
        "Nombre de photons par d#acute{e}sint#acute{e}gration (PreContainer);N_{#gamma} vers eau (+z);Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    h_pre_nPhotons->SetLineColor(kOrange+1);
    h_pre_nPhotons->SetFillColor(kOrange-9);
    h_pre_nPhotons->SetFillStyle(3001);
    treePreContainer->Draw("nPhotons>>h_pre_nPhotons", "", "");
    
    // --- Histogramme 1.2 : Somme énergie photons ---
    c1->cd(2);
    gPad->SetLogy();
    TH1D* h_pre_sumEPhotons = new TH1D("h_pre_sumEPhotons", 
        "Somme des #acute{e}nergies des photons (PreContainer);#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 5000);
    h_pre_sumEPhotons->SetLineColor(kOrange+1);
    h_pre_sumEPhotons->SetFillColor(kOrange-9);
    h_pre_sumEPhotons->SetFillStyle(3001);
    treePreContainer->Draw("sumEPhotons_keV>>h_pre_sumEPhotons", "sumEPhotons_keV>0", "");
    
    // --- Histogramme 1.3 : Nombre d'électrons ---
    c1->cd(3);
    gPad->SetLogy();
    TH1D* h_pre_nElectrons = new TH1D("h_pre_nElectrons", 
        "Nombre d'#acute{e}lectrons par d#acute{e}sint#acute{e}gration (PreContainer);N_{e^{-}} vers eau (+z);Nombre d'#acute{e}v#acute{e}nements", 
        20, 0, 20);
    h_pre_nElectrons->SetLineColor(kGreen+2);
    h_pre_nElectrons->SetFillColor(kGreen-9);
    h_pre_nElectrons->SetFillStyle(3001);
    treePreContainer->Draw("nElectrons>>h_pre_nElectrons", "", "");
    
    // --- Histogramme 1.4 : Somme énergie électrons ---
    c1->cd(4);
    gPad->SetLogy();
    TH1D* h_pre_sumEElectrons = new TH1D("h_pre_sumEElectrons", 
        "Somme des #acute{e}nergies des #acute{e}lectrons (PreContainer);#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 2000);
    h_pre_sumEElectrons->SetLineColor(kGreen+2);
    h_pre_sumEElectrons->SetFillColor(kGreen-9);
    h_pre_sumEElectrons->SetFillStyle(3001);
    treePreContainer->Draw("sumEElectrons_keV>>h_pre_sumEElectrons", "sumEElectrons_keV>0", "");
    
    c1->Update();
    c1->SaveAs("histos_precontainer.png");
    c1->SaveAs("histos_precontainer.pdf");
    
    std::cout << "\n>>> Canvas 1 sauvegardé: histos_precontainer.png/pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 2 : Plan PostContainer - Photons rétrodiffusés (depuis eau)
    // ═══════════════════════════════════════════════════════════════════════
    
    TCanvas* c2 = new TCanvas("c2", "Plan PostContainerPlane - Photons depuis eau", 1400, 500);
    c2->Divide(2, 1);
    
    // --- Histogramme 2.1 : Nombre de photons rétrodiffusés ---
    c2->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nPhotons_back = new TH1D("h_post_nPhotons_back", 
        "Nombre de photons depuis l'eau par d#acute{e}sint#acute{e}gration;N_{#gamma} depuis eau (-z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nPhotons_back->SetLineColor(kViolet+1);
    h_post_nPhotons_back->SetFillColor(kViolet-9);
    h_post_nPhotons_back->SetFillStyle(3001);
    treePostContainer->Draw("nPhotons_back>>h_post_nPhotons_back", "", "");
    
    // --- Histogramme 2.2 : Somme énergie photons rétrodiffusés ---
    c2->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEPhotons_back = new TH1D("h_post_sumEPhotons_back", 
        "Somme des #acute{e}nergies des photons depuis l'eau;#SigmaE_{#gamma} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1500);
    h_post_sumEPhotons_back->SetLineColor(kViolet+1);
    h_post_sumEPhotons_back->SetFillColor(kViolet-9);
    h_post_sumEPhotons_back->SetFillStyle(3001);
    treePostContainer->Draw("sumEPhotons_back_keV>>h_post_sumEPhotons_back", "sumEPhotons_back_keV>0", "");
    
    c2->Update();
    c2->SaveAs("histos_postcontainer_photons.png");
    c2->SaveAs("histos_postcontainer_photons.pdf");
    
    std::cout << ">>> Canvas 2 sauvegardé: histos_postcontainer_photons.png/pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 3 : Plan PostContainer - Électrons vers l'eau (+z)
    // ═══════════════════════════════════════════════════════════════════════
    
    TCanvas* c3 = new TCanvas("c3", "Plan PostContainerPlane - Electrons vers eau (+z)", 1400, 500);
    c3->Divide(2, 1);
    
    // --- Histogramme 3.1 : Nombre d'électrons vers eau ---
    c3->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nElectrons_fwd = new TH1D("h_post_nElectrons_fwd", 
        "Nombre d'#acute{e}lectrons vers l'eau par d#acute{e}sint#acute{e}gration;N_{e^{-}} vers eau (+z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nElectrons_fwd->SetLineColor(kBlue+1);
    h_post_nElectrons_fwd->SetFillColor(kBlue-9);
    h_post_nElectrons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("nElectrons_fwd>>h_post_nElectrons_fwd", "", "");
    
    // --- Histogramme 3.2 : Somme énergie électrons vers eau ---
    c3->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEElectrons_fwd = new TH1D("h_post_sumEElectrons_fwd", 
        "Somme des #acute{e}nergies des #acute{e}lectrons vers l'eau;#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1000);
    h_post_sumEElectrons_fwd->SetLineColor(kBlue+1);
    h_post_sumEElectrons_fwd->SetFillColor(kBlue-9);
    h_post_sumEElectrons_fwd->SetFillStyle(3001);
    treePostContainer->Draw("sumEElectrons_fwd_keV>>h_post_sumEElectrons_fwd", "sumEElectrons_fwd_keV>0", "");
    
    c3->Update();
    c3->SaveAs("histos_postcontainer_electrons_fwd.png");
    c3->SaveAs("histos_postcontainer_electrons_fwd.pdf");
    
    std::cout << ">>> Canvas 3 sauvegardé: histos_postcontainer_electrons_fwd.png/pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 4 : Plan PostContainer - Électrons depuis l'eau (-z)
    // ═══════════════════════════════════════════════════════════════════════
    
    TCanvas* c4 = new TCanvas("c4", "Plan PostContainerPlane - Electrons depuis eau (-z)", 1400, 500);
    c4->Divide(2, 1);
    
    // --- Histogramme 4.1 : Nombre d'électrons depuis eau ---
    c4->cd(1);
    gPad->SetLogy();
    TH1D* h_post_nElectrons_back = new TH1D("h_post_nElectrons_back", 
        "Nombre d'#acute{e}lectrons depuis l'eau par d#acute{e}sint#acute{e}gration;N_{e^{-}} depuis eau (-z);Nombre d'#acute{e}v#acute{e}nements", 
        10, 0, 10);
    h_post_nElectrons_back->SetLineColor(kRed+1);
    h_post_nElectrons_back->SetFillColor(kRed-9);
    h_post_nElectrons_back->SetFillStyle(3001);
    treePostContainer->Draw("nElectrons_back>>h_post_nElectrons_back", "", "");
    
    // --- Histogramme 4.2 : Somme énergie électrons depuis eau ---
    c4->cd(2);
    gPad->SetLogy();
    TH1D* h_post_sumEElectrons_back = new TH1D("h_post_sumEElectrons_back", 
        "Somme des #acute{e}nergies des #acute{e}lectrons depuis l'eau;#SigmaE_{e^{-}} (keV);Nombre d'#acute{e}v#acute{e}nements", 
        100, 0, 1000);
    h_post_sumEElectrons_back->SetLineColor(kRed+1);
    h_post_sumEElectrons_back->SetFillColor(kRed-9);
    h_post_sumEElectrons_back->SetFillStyle(3001);
    treePostContainer->Draw("sumEElectrons_back_keV>>h_post_sumEElectrons_back", "sumEElectrons_back_keV>0", "");
    
    c4->Update();
    c4->SaveAs("histos_postcontainer_electrons_back.png");
    c4->SaveAs("histos_postcontainer_electrons_back.pdf");
    
    std::cout << ">>> Canvas 4 sauvegardé: histos_postcontainer_electrons_back.png/pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CANVAS 5 : Résumé complet (2x5)
    // ═══════════════════════════════════════════════════════════════════════
    
    TCanvas* c5 = new TCanvas("c5", "Résumé Plans Container", 1800, 1200);
    c5->Divide(2, 5);
    
    // Ligne 1 : PreContainer Photons
    c5->cd(1); gPad->SetLogy();
    h_pre_nPhotons->Draw();
    c5->cd(2); gPad->SetLogy();
    h_pre_sumEPhotons->Draw();
    
    // Ligne 2 : PreContainer Électrons
    c5->cd(3); gPad->SetLogy();
    h_pre_nElectrons->Draw();
    c5->cd(4); gPad->SetLogy();
    h_pre_sumEElectrons->Draw();
    
    // Ligne 3 : PostContainer Photons (back)
    c5->cd(5); gPad->SetLogy();
    h_post_nPhotons_back->Draw();
    c5->cd(6); gPad->SetLogy();
    h_post_sumEPhotons_back->Draw();
    
    // Ligne 4 : PostContainer Électrons (fwd)
    c5->cd(7); gPad->SetLogy();
    h_post_nElectrons_fwd->Draw();
    c5->cd(8); gPad->SetLogy();
    h_post_sumEElectrons_fwd->Draw();
    
    // Ligne 5 : PostContainer Électrons (back)
    c5->cd(9); gPad->SetLogy();
    h_post_nElectrons_back->Draw();
    c5->cd(10); gPad->SetLogy();
    h_post_sumEElectrons_back->Draw();
    
    c5->Update();
    c5->SaveAs("histos_container_summary.png");
    c5->SaveAs("histos_container_summary.pdf");
    
    std::cout << ">>> Canvas 5 sauvegardé: histos_container_summary.png/pdf" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Affichage des statistiques
    // ═══════════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    STATISTIQUES DES HISTOGRAMMES                ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ PRECONTAINER (avant eau, Air, GAP=0)                           ║" << std::endl;
    std::cout << "║   Photons vers eau:    Mean = " << h_pre_nPhotons->GetMean() 
              << "  RMS = " << h_pre_nPhotons->GetRMS() << std::endl;
    std::cout << "║   Energie photons:     Mean = " << h_pre_sumEPhotons->GetMean() 
              << " keV" << std::endl;
    std::cout << "║   Electrons vers eau:  Mean = " << h_pre_nElectrons->GetMean() 
              << "  RMS = " << h_pre_nElectrons->GetRMS() << std::endl;
    std::cout << "║   Energie electrons:   Mean = " << h_pre_sumEElectrons->GetMean() 
              << " keV" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ POSTCONTAINER (après eau, W_PETG, GAP=0)                       ║" << std::endl;
    std::cout << "║   Photons depuis eau:  Mean = " << h_post_nPhotons_back->GetMean() 
              << "  RMS = " << h_post_nPhotons_back->GetRMS() << std::endl;
    std::cout << "║   Energie photons:     Mean = " << h_post_sumEPhotons_back->GetMean() 
              << " keV" << std::endl;
    std::cout << "║   Electrons vers eau:  Mean = " << h_post_nElectrons_fwd->GetMean() 
              << "  RMS = " << h_post_nElectrons_fwd->GetRMS() << std::endl;
    std::cout << "║   Electrons depuis eau: Mean = " << h_post_nElectrons_back->GetMean() 
              << "  RMS = " << h_post_nElectrons_back->GetRMS() << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\n=== Script terminé avec succès ===" << std::endl;
    std::cout << "Fichiers générés:" << std::endl;
    std::cout << "  - histos_precontainer.png/pdf" << std::endl;
    std::cout << "  - histos_postcontainer_photons.png/pdf" << std::endl;
    std::cout << "  - histos_postcontainer_electrons_fwd.png/pdf" << std::endl;
    std::cout << "  - histos_postcontainer_electrons_back.png/pdf" << std::endl;
    std::cout << "  - histos_container_summary.png/pdf" << std::endl;
}
