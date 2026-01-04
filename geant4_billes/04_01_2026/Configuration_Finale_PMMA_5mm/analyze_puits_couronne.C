// =============================================================================
// SCRIPT D'ANALYSE - PUITS COURONNE (version simplifiee)
// =============================================================================
// 
// Usage: root -l analyze_puits_couronne.C
//    ou: root -l -b -q 'analyze_puits_couronne.C("puits_couronne.root")'
//
// =============================================================================

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TPaveStats.h>
#include <iostream>
#include <iomanip>

// Couleurs pour les anneaux
const int ringColors[] = {kRed, kOrange+1, kGreen+2, kBlue, kViolet};

// Noms des raies gamma Eu-152
const char* gammaLineNames[] = {
    "122 keV", "245 keV", "344 keV", "411 keV", "444 keV",
    "779 keV", "867 keV", "964 keV", "1086 keV", "1112 keV", "1408 keV"
};
const int nGammaLines = 11;

void analyze_puits_couronne(const char* filename = "puits_couronne.root")
{
    // Configuration du style
    gStyle->SetOptStat(10);        // 10 = seulement Entries
    gStyle->SetHistLineWidth(3);   // Lignes plus epaisses
    gStyle->SetTitleFont(62, "");  // Titre en gras (62 = Helvetica Bold)
    gStyle->SetTitleFontSize(0.06);// Taille du titre
    
    // ==========================================================================
    // OUVERTURE DU FICHIER
    // ==========================================================================
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "ERREUR: Impossible d'ouvrir " << filename << std::endl;
        return;
    }
    
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "     ANALYSE DU FICHIER: " << filename << "\n";
    std::cout << "================================================================\n\n";
    
    // ==========================================================================
    // 1. HISTOGRAMMES DE DOSE PAR ANNEAU (nGy)
    // ==========================================================================
    
    TCanvas* c_dose = new TCanvas("c_dose", "Dose par anneau", 1200, 800);
    c_dose->Divide(3, 2);
    
    TH1D* h_dose_ring[5];
    TH1D* h_dose_total = nullptr;
    
    // =========================================
    // PARAMETRES DU PAVE DE STATISTIQUES
    // Position des coins (coordonnees NDC 0-1)
    // =========================================
    double statX1 = 0.60;  // Coin gauche
    double statX2 = 0.90;  // Coin droit
    double statY1 = 0.78;  // Coin bas
    double statY2 = 0.90;  // Coin haut
    double statTextSize = 0.04;  // Taille police
    
    for (int i = 0; i < 5; i++) {
        TString hname = Form("h_dose_ring%d", i);
        h_dose_ring[i] = (TH1D*)file->Get(hname);
        
        if (h_dose_ring[i]) {
            c_dose->cd(i + 1);
            gPad->SetLogy();
            
            // Anneau 0 : 0-1 nGy, Anneaux 1-4 : 0-0.3 nGy
            if (i == 0) {
                h_dose_ring[i]->GetXaxis()->SetRangeUser(0., 1.);
            } else {
                h_dose_ring[i]->GetXaxis()->SetRangeUser(0., 0.3);
            }
            
            h_dose_ring[i]->SetLineColor(ringColors[i]);
            h_dose_ring[i]->SetFillColor(ringColors[i]);
            h_dose_ring[i]->SetFillStyle(3004);
            h_dose_ring[i]->SetLineWidth(3);
            h_dose_ring[i]->GetXaxis()->SetTitle("Dose [nGy]");
            h_dose_ring[i]->GetYaxis()->SetTitle("Counts");
            h_dose_ring[i]->GetXaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetXaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->SetTitle(Form("Anneau %d", i));
            h_dose_ring[i]->Draw();
            
            // Mise a jour et configuration du pave de stats
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)h_dose_ring[i]->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(statX1);
                stats->SetX2NDC(statX2);
                stats->SetY1NDC(statY1);
                stats->SetY2NDC(statY2);
                stats->SetTextSize(statTextSize);
                stats->SetTextFont(62);  // Helvetica Bold
                gPad->Modified();
            }
        }
    }

    // Dose totale
    h_dose_total = (TH1D*)file->Get("h_dose_total");
    if (h_dose_total) {
        c_dose->cd(6);
        gPad->SetLogy();
        h_dose_total->GetXaxis()->SetRangeUser(0., 0.3);
        h_dose_total->SetLineColor(kBlack);
        h_dose_total->SetLineWidth(3);
        h_dose_total->GetXaxis()->SetTitle("Dose totale [nGy]");
        h_dose_total->GetYaxis()->SetTitle("Counts");
        h_dose_total->GetXaxis()->SetTitleSize(0.05);
        h_dose_total->GetYaxis()->SetTitleSize(0.05);
        h_dose_total->GetXaxis()->SetLabelSize(0.05);
        h_dose_total->GetYaxis()->SetLabelSize(0.05);
        h_dose_total->SetTitle("TOTAL");
        h_dose_total->Draw();
        
        // Mise a jour et configuration du pave de stats
        gPad->Update();
        TPaveStats* stats = (TPaveStats*)h_dose_total->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(statX1);
            stats->SetX2NDC(statX2);
            stats->SetY1NDC(statY1);
            stats->SetY2NDC(statY2);
            stats->SetTextSize(statTextSize);
            stats->SetTextFont(62);
            gPad->Modified();
        }
    }
    
    c_dose->Update();
    c_dose->SaveAs("dose_par_anneau.png");
    std::cout << "=> Sauvegarde: dose_par_anneau.png\n" << std::endl;
    
    // ==========================================================================
    // 2. TAUX D'ABSORPTION PAR RAIE GAMMA
    // ==========================================================================
    
    gStyle->SetOptStat(0);  // Desactiver stats SEULEMENT pour cet histogramme
    
    TTree* gamma_lines = (TTree*)file->Get("gamma_lines");
    if (gamma_lines) {
        
        Int_t lineIndex;
        Double_t filterAbsRate;
        
        gamma_lines->SetBranchAddress("lineIndex", &lineIndex);
        gamma_lines->SetBranchAddress("filterAbsRate", &filterAbsRate);
        
        // Graphique des taux d'absorption
        TCanvas* c_abs = new TCanvas("c_abs", "Taux d'absorption par raie", 900, 600);
        c_abs->SetBottomMargin(0.15);  // Marge en bas pour les labels X
        c_abs->SetLeftMargin(0.12);    // Marge a gauche pour les labels Y
        
        TH1D* h_abs_filter = new TH1D("h_abs_filter", 
            "Taux d'absorption dans le filtre W/PETG;Raie gamma;Taux d'absorption (%)", 
            nGammaLines, 0, nGammaLines);
        
        for (Long64_t i = 0; i < gamma_lines->GetEntries(); i++) {
            gamma_lines->GetEntry(i);
            h_abs_filter->SetBinContent(lineIndex + 1, filterAbsRate);
            h_abs_filter->GetXaxis()->SetBinLabel(lineIndex + 1, gammaLineNames[lineIndex]);
        }
        
        h_abs_filter->SetFillColor(kOrange+1);
        h_abs_filter->SetLineColor(kOrange+7);
        h_abs_filter->SetLineWidth(2);
        h_abs_filter->SetMinimum(0);
        h_abs_filter->SetMaximum(100);
        
        // Axe X : augmenter la distance etiquette-axe
        h_abs_filter->GetXaxis()->SetLabelSize(0.045);
        h_abs_filter->GetXaxis()->SetLabelOffset(0.02);
        h_abs_filter->GetXaxis()->SetTitleOffset(2.0);

        // Axe Y : diminuer la distance etiquette-axe
        h_abs_filter->GetYaxis()->SetLabelSize(0.045);
        h_abs_filter->GetYaxis()->SetTitleSize(0.05);
        h_abs_filter->GetYaxis()->SetTitleOffset(0.8);
        h_abs_filter->GetYaxis()->SetLabelOffset(0.005);
        
        h_abs_filter->Draw("bar");
        
        TLatex* latex = new TLatex();
        latex->SetNDC();
        latex->SetTextSize(0.04);
        latex->DrawLatex(0.45, 0.85, "Filtre W/PETG (75%/25%), #Deltax = 5 mm");
        
        c_abs->SaveAs("taux_absorption.png");
        std::cout << "=> Sauvegarde: taux_absorption.png\n" << std::endl;
    }
    
    // ==========================================================================
    // RESUME
    // ==========================================================================
    
    std::cout << "================================================================\n";
    std::cout << "                    ANALYSE TERMINEE                            \n";
    std::cout << "================================================================\n";
    std::cout << "  Fichiers generes:                                             \n";
    std::cout << "    - dose_par_anneau.png                                       \n";
    std::cout << "    - taux_absorption.png                                       \n";
    std::cout << "================================================================\n";
}
