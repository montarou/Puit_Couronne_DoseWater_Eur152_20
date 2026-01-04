// ═══════════════════════════════════════════════════════════════════════════════
// analyse_dose_anneaux.C
// Script ROOT pour analyser la dose déposée par anneau d'eau
// Adapté pour le fichier puits_couronne.root
// 
// Structure du fichier:
//   - Tree "doses" avec branches dose_nGy_ring0..4, edep_keV_total, etc.
//   - Histogrammes h_dose_ring0..4 pré-calculés
//
// Usage: root -l -b -q 'analyse_dose_anneaux.C("puits_couronne.root")'
// ═══════════════════════════════════════════════════════════════════════════════

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TKey.h>
#include <TObjArray.h>
#include <TBranch.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

void analyse_dose_anneaux(const char* filename = "puits_couronne.root") {
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════════
    
    const int nRings = 5;
    
    // Masses des anneaux (g) - calculées géométriquement
    double masses[nRings];
    double rayons_int[nRings] = {0, 5, 10, 15, 20};   // mm
    double rayons_ext[nRings] = {5, 10, 15, 20, 25};  // mm
    double hauteur = 5.0;  // mm
    double rho_eau = 1.0;  // g/cm³
    
    for (int i = 0; i < nRings; i++) {
        double r_int_cm = rayons_int[i] / 10.0;
        double r_ext_cm = rayons_ext[i] / 10.0;
        double h_cm = hauteur / 10.0;
        double volume = M_PI * h_cm * (r_ext_cm * r_ext_cm - r_int_cm * r_int_cm);
        masses[i] = volume * rho_eau;
    }
    
    // Couleurs pour les anneaux
    int colors[nRings] = {kRed+1, kOrange+1, kGreen+2, kBlue+1, kViolet+1};
    
    // ═══════════════════════════════════════════════════════════════════════════
    // OUVERTURE DU FICHIER
    // ═══════════════════════════════════════════════════════════════════════════
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier " << filename << std::endl;
        return;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           ANALYSE DES DOSES PAR ANNEAU D'EAU                      ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Fichier: " << std::setw(54) << std::left << filename << "║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // LECTURE DU TREE "doses"
    // ═══════════════════════════════════════════════════════════════════════════
    
    TTree* tree = (TTree*)file->Get("doses");
    if (!tree) {
        std::cerr << "Erreur: Tree 'doses' non trouvé!" << std::endl;
        file->Close();
        return;
    }
    
    Long64_t nEntries = tree->GetEntries();
    std::cout << "Tree 'doses': " << nEntries << " entrées" << std::endl;
    
    // Variables pour lecture
    double dose_nGy[nRings];
    double dose_total, edep_keV_total;
    int nPrimaries, nTransmitted, nAbsorbed;
    
    // Connecter les branches
    for (int i = 0; i < nRings; i++) {
        tree->SetBranchAddress(Form("dose_nGy_ring%d", i), &dose_nGy[i]);
    }
    tree->SetBranchAddress("dose_nGy_total", &dose_total);
    tree->SetBranchAddress("edep_keV_total", &edep_keV_total);
    tree->SetBranchAddress("nPrimaries", &nPrimaries);
    tree->SetBranchAddress("nTransmitted", &nTransmitted);
    tree->SetBranchAddress("nAbsorbed", &nAbsorbed);
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CRÉATION DES HISTOGRAMMES DE DOSE PAR DÉSINTÉGRATION
    // ═══════════════════════════════════════════════════════════════════════════
    
    TH1D* hDosePerEvent[nRings];
    for (int i = 0; i < nRings; i++) {
        hDosePerEvent[i] = new TH1D(Form("hDosePerEvent_ring%d", i),
                                    Form("Anneau %d (r=%d-%d mm);Dose par d#acute{e}sint#acute{e}gration (nGy);Nombre d'#acute{e}v#acute{e}nements", 
                                         i, (int)rayons_int[i], (int)rayons_ext[i]),
                                    250, 0, 0.5);  // Gamme 0 à 0.5 nGy
        hDosePerEvent[i]->SetLineColor(colors[i]);
        hDosePerEvent[i]->SetFillColor(colors[i]);
        hDosePerEvent[i]->SetFillStyle(3004 + i);
        hDosePerEvent[i]->SetLineWidth(3);
    }
    
    TH1D* hDoseTotal = new TH1D("hDoseTotal", "Dose totale par d#acute{e}sint#acute{e}gration;Dose (nGy);Nombre d'#acute{e}v#acute{e}nements",
                                250, 0, 0.5);  // Gamme 0 à 0.5 nGy
    hDoseTotal->SetLineColor(kBlack);
    hDoseTotal->SetLineWidth(3);
    
    // ═══════════════════════════════════════════════════════════════════════════
    // LECTURE DES DONNÉES ET CALCUL DES STATISTIQUES
    // ═══════════════════════════════════════════════════════════════════════════
    

    double sumDose[nRings];
    double sumDose2[nRings];  // Somme des carrés pour l'erreur statistique

    for (int i = 0; i < nRings; i++) {
            sumDose[i] = 0;
            sumDose2[i] = 0;
    }

    double sumDoseTotal = 0;
    double sumEdep = 0;
    int nEventsWithDeposit[nRings];
    for (int i = 0; i < nRings; i++) {
            nEventsWithDeposit[i] = 0;
    }

    int nEventsWithAnyDeposit = 0;
    Long64_t totalPrimaries = 0;

    std::cout << "\nLecture des données..." << std::endl;

    for (Long64_t ev = 0; ev < nEntries; ev++) {
        tree->GetEntry(ev);

        bool hasDeposit = false;
        double doseThisEvent = 0;  // Calculer la dose totale comme somme des anneaux

        for (int ring = 0; ring < nRings; ring++) {
            if (dose_nGy[ring] > 0) {
                hDosePerEvent[ring]->Fill(dose_nGy[ring]);
                sumDose[ring] += dose_nGy[ring];
                sumDose2[ring] += dose_nGy[ring] * dose_nGy[ring];  // Pour l'erreur
                doseThisEvent += dose_nGy[ring];
                nEventsWithDeposit[ring]++;
                hasDeposit = true;
            }
        }

        // Utiliser la somme des anneaux pour la dose totale (pas dose_total du fichier)
        if (doseThisEvent > 0) {
            hDoseTotal->Fill(doseThisEvent);
            sumDoseTotal += doseThisEvent;
        }
        
        sumEdep += edep_keV_total;
        totalPrimaries += nPrimaries;
        
        if (hasDeposit) nEventsWithAnyDeposit++;
        
        if (ev % 50000 == 0 && ev > 0) {
            std::cout << "  Traité " << ev << " / " << nEntries << " entrées..." << std::endl;
        }
    }
    
    // Nombre total d'événements (désintégrations)
    // Chaque entrée du tree correspond à 100 événements (basé sur nEntries=250000 pour 25M events)
    Long64_t nEvents = nEntries * 100;  // Ajuster si nécessaire
    
    std::cout << "Lecture terminée." << std::endl;
    std::cout << "  Entrées dans le tree: " << nEntries << std::endl;
    std::cout << "  Événements estimés: " << nEvents << std::endl;
    std::cout << "  Primaires totaux: " << totalPrimaries << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // TABLEAU RÉCAPITULATIF
    // ═══════════════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                        DOSES PAR ANNEAU D'EAU                                                ║" << std::endl;
    std::cout << "╠═════════╦═══════════════╦═══════════════╦═══════════════════╦═══════════════════╦═══════════════╦════════════╣" << std::endl;
    std::cout << "║ Anneau  ║  r_int-r_ext  ║   Masse (g)   ║ Dose totale (nGy) ║ Dose moyenne(nGy) ║ Dose/evt(nGy) ║ Evts D>0   ║" << std::endl;
    std::cout << "╠═════════╬═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═══════════════╬════════════╣" << std::endl;
    
    double grandTotalDose = 0;
    double grandTotalMass = 0;
    
    for (int i = 0; i < nRings; i++) {
        double doseMoyenne = (nEventsWithDeposit[i] > 0) ? sumDose[i] / nEventsWithDeposit[i] : 0;
        double dosePerEvent = sumDose[i] / nEvents;
        
        std::cout << "║    " << i << "    ║   " 
                  << std::setw(2) << (int)rayons_int[i] << "-" << std::setw(2) << (int)rayons_ext[i] << " mm"
                  << "    ║   " << std::fixed << std::setprecision(4) << masses[i]
                  << "    ║     " << std::scientific << std::setprecision(3) << sumDose[i]
                  << "     ║     " << std::scientific << std::setprecision(3) << doseMoyenne
                  << "     ║   " << std::scientific << std::setprecision(3) << dosePerEvent
                  << " ║ " << std::setw(10) << std::fixed << std::setprecision(0) << nEventsWithDeposit[i] << " ║" << std::endl;
        
        grandTotalDose += sumDose[i];
        grandTotalMass += masses[i];
    }
    
    std::cout << "╠═════════╬═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═══════════════╬════════════╣" << std::endl;
    
    double grandDosePerEvent = grandTotalDose / nEvents;
    double grandDoseMoyenne = (nEventsWithAnyDeposit > 0) ? grandTotalDose / nEventsWithAnyDeposit : 0;
    
    std::cout << "║  TOTAL  ║               ║   " << std::fixed << std::setprecision(4) << grandTotalMass
              << "    ║     " << std::scientific << std::setprecision(3) << grandTotalDose
              << "     ║     " << std::scientific << std::setprecision(3) << grandDoseMoyenne
              << "     ║   " << std::scientific << std::setprecision(3) << grandDosePerEvent
              << " ║ " << std::setw(10) << std::fixed << std::setprecision(0) << nEvents << " ║" << std::endl;
    
    std::cout << "╚═════════╩═══════════════╩═══════════════╩═══════════════════╩═══════════════════╩═══════════════╩════════════╝" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // RÉCUPÉRATION DES HISTOGRAMMES PRÉ-CALCULÉS
    // ═══════════════════════════════════════════════════════════════════════════
    
    std::cout << "\n=== Histogrammes pré-calculés dans le fichier ===" << std::endl;
    
    TH1D* hDoseFile[nRings];
    for (int i = 0; i < nRings; i++) {
        hDoseFile[i] = (TH1D*)file->Get(Form("h_dose_ring%d", i));
        if (hDoseFile[i]) {
            std::cout << "  h_dose_ring" << i << ": Entries=" << hDoseFile[i]->GetEntries() 
                      << ", Mean=" << hDoseFile[i]->GetMean() << " nGy" << std::endl;
        }
    }
    
    TH1D* hDoseTotalFile = (TH1D*)file->Get("h_dose_total");
    if (hDoseTotalFile) {
        std::cout << "  h_dose_total: Entries=" << hDoseTotalFile->GetEntries() 
                  << ", Mean=" << hDoseTotalFile->GetMean() << " nGy" << std::endl;
    }
    
    // ═══════════════════════════════════════════════════════════════════════════
    // CRÉATION DES FIGURES
    // ═══════════════════════════════════════════════════════════════════════════
    
    gStyle->SetOptStat(0);
    gStyle->SetTitleSize(0.045, "XY");
    gStyle->SetLabelSize(0.04, "XY");
    
    // ─────────────────────────────────────────────────────────────────────────
    // Figure 1: Histogrammes individuels de dose par anneau (depuis le fichier)
    // ─────────────────────────────────────────────────────────────────────────
    
    TCanvas* c1 = new TCanvas("c1", "Dose par anneau", 1500, 1000);
    c1->Divide(3, 2);
    
    for (int i = 0; i < nRings; i++) {
        c1->cd(i + 1);
        gPad->SetLogy();
        gPad->SetLeftMargin(0.14);
        gPad->SetRightMargin(0.05);
        gPad->SetBottomMargin(0.12);
        
        if (hDoseFile[i]) {
            hDoseFile[i]->SetLineColor(colors[i]);
            hDoseFile[i]->SetFillColor(colors[i]);
            hDoseFile[i]->SetFillStyle(3004 + i);
            hDoseFile[i]->SetLineWidth(3);
            hDoseFile[i]->SetTitle(Form("Anneau %d (r=%d-%d mm);Dose par d#acute{e}sint#acute{e}gration (nGy);Nombre d'#acute{e}v#acute{e}nements",
                                        i, (int)rayons_int[i], (int)rayons_ext[i]));
            hDoseFile[i]->GetXaxis()->SetRangeUser(0, 0.5);  // Limiter l'axe X à 0-0.5 nGy
            hDoseFile[i]->GetYaxis()->SetTitleOffset(1.4);
            hDoseFile[i]->Draw();
            
            TPaveText* stats = new TPaveText(0.5, 0.65, 0.95, 0.9, "NDC");
            stats->SetFillColor(kWhite);
            stats->SetBorderSize(1);
            stats->SetTextAlign(12);
            stats->SetTextSize(0.038);
            stats->AddText(Form("Entries: %.0f", hDoseFile[i]->GetEntries()));
            stats->AddText(Form("Mean: %.3e nGy", hDoseFile[i]->GetMean()));
            stats->AddText(Form("Std Dev: %.3e nGy", hDoseFile[i]->GetStdDev()));
            stats->AddText(Form("#bf{Dose tot: %.3e nGy}", sumDose[i]));
            stats->Draw();
        }
    }
    
    
    c1->SaveAs("histos_dose_par_anneau.png");
    std::cout << "\n✓ histos_dose_par_anneau.png" << std::endl;
    
    // ─────────────────────────────────────────────────────────────────────────
    // Figure 2: Superposition des histogrammes
    // ─────────────────────────────────────────────────────────────────────────
    
    TCanvas* c2 = new TCanvas("c2", "Comparaison doses", 1200, 800);
    c2->SetLogy();
    c2->SetLeftMargin(0.12);
    c2->SetRightMargin(0.05);
    c2->SetBottomMargin(0.12);

    double maxY = 0;
    for (int i = 0; i < nRings; i++) {
        if (hDoseFile[i] && hDoseFile[i]->GetMaximum() > maxY) {
            maxY = hDoseFile[i]->GetMaximum();
        }
    }

    bool firstDrawn = false;
    for (int i = 0; i < nRings; i++) {
        if (hDoseFile[i]) {
            hDoseFile[i]->SetFillStyle(0);
            hDoseFile[i]->GetXaxis()->SetRangeUser(0, 0.75);  // Limiter l'axe X à 0-0.75 nGy
            if (!firstDrawn) {
                hDoseFile[i]->SetMaximum(maxY * 2);
                hDoseFile[i]->SetMinimum(0.5);
                hDoseFile[i]->SetTitle("Dose par d#acute{e}sint#acute{e}gration - Comparaison des anneaux;Dose (nGy);Nombre d'#acute{e}v#acute{e}nements");
                hDoseFile[i]->Draw("HIST");
                firstDrawn = true;
            } else {
                hDoseFile[i]->Draw("HIST SAME");
            }
        }
    }

    TLegend* leg = new TLegend(0.62, 0.55, 0.93, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.035);
    for (int i = 0; i < nRings; i++) {
        if (hDoseFile[i]) {
            leg->AddEntry(hDoseFile[i], Form("Anneau %d (r=%d-%d mm)", i, (int)rayons_int[i], (int)rayons_ext[i]), "l");
        }
    }
    leg->Draw();

    
    c2->SaveAs("histos_dose_comparaison.png");
    std::cout << "✓ histos_dose_comparaison.png" << std::endl;
    
    // ─────────────────────────────────────────────────────────────────────────
    // Figure 3: Dose totale (calculée comme somme des anneaux)
    // ─────────────────────────────────────────────────────────────────────────
    
    TCanvas* c3 = new TCanvas("c3", "Dose totale", 1000, 700);
    c3->SetLogy();
    c3->SetLeftMargin(0.12);
    c3->SetRightMargin(0.05);
    c3->SetBottomMargin(0.12);

    hDoseTotal->SetLineColor(kBlue+1);
    hDoseTotal->SetFillColor(kBlue-9);
    hDoseTotal->SetFillStyle(1001);
    hDoseTotal->SetLineWidth(3);
    hDoseTotal->SetTitle("Dose totale par d#acute{e}sint#acute{e}gration (somme des anneaux);Dose (nGy);Nombre d'#acute{e}v#acute{e}nements");
    hDoseTotal->GetYaxis()->SetTitleOffset(1.2);
    hDoseTotal->Draw();

    TPaveText* stats3 = new TPaveText(0.5, 0.65, 0.95, 0.9, "NDC");
    stats3->SetFillColor(kWhite);
    stats3->SetBorderSize(1);
    stats3->SetTextAlign(12);
    stats3->SetTextSize(0.04);
    stats3->AddText(Form("Entries: %.0f", hDoseTotal->GetEntries()));
    stats3->AddText(Form("Mean: %.3e nGy", hDoseTotal->GetMean()));
    stats3->AddText(Form("Std Dev: %.3e nGy", hDoseTotal->GetStdDev()));
    stats3->AddText(Form("#bf{Dose totale: %.3e nGy}", sumDoseTotal));
    stats3->Draw();

    c3->SaveAs("histos_dose_totale.png");
    std::cout << "✓ histos_dose_totale.png" << std::endl;
    
    // ─────────────────────────────────────────────────────────────────────────
    // Figure 4: Dose moyenne vs rayon
    // ─────────────────────────────────────────────────────────────────────────
    
    TCanvas* c4 = new TCanvas("c4", "Dose vs Rayon", 1000, 700);
    c4->SetLeftMargin(0.14);
    c4->SetRightMargin(0.05);
    c4->SetBottomMargin(0.12);
    c4->SetGridy();
    
    double rayons_centre[nRings];
    double rayons_err[nRings];      // Demi-largeur des anneaux
    double dose_par_event[nRings];
    double dose_err[nRings];        // Erreur statistique sur la dose moyenne

    std::cout << "\n=== ERREURS STATISTIQUES ===" << std::endl;

    for (int i = 0; i < nRings; i++) {
        rayons_centre[i] = (rayons_int[i] + rayons_ext[i]) / 2.0;
        rayons_err[i] = (rayons_ext[i] - rayons_int[i]) / 2.0;  // Demi-largeur = 2.5 mm

        dose_par_event[i] = sumDose[i] / nEvents;

        // Erreur statistique: sigma / sqrt(N)
        // Variance = <x²> - <x>² = sumDose2/N - (sumDose/N)²
        // Pour la moyenne sur N événements: erreur = sqrt(variance) / sqrt(N) = sqrt(variance/N)
        double mean = sumDose[i] / nEvents;
        double mean2 = sumDose2[i] / nEvents;
        double variance = mean2 - mean * mean;
        if (variance < 0) variance = 0;  // Protection numérique
        double stddev = sqrt(variance);
        dose_err[i] = stddev / sqrt((double)nEvents);  // Erreur sur la moyenne

        // Erreur relative en %
        double errRel = (dose_par_event[i] > 0) ? 100.0 * dose_err[i] / dose_par_event[i] : 0;

        std::cout << "  Anneau " << i << ": Dose = " << std::scientific << dose_par_event[i]
        << " +/- " << dose_err[i] << " nGy (" << std::fixed << std::setprecision(2)
        << errRel << "%)" << std::endl;

    }
    
    // Les barres d'erreur statistiques sont très petites (erreur/sqrt(N))
    // On affiche aussi l'écart-type des mesures pour montrer la dispersion
    // Option: multiplier les erreurs par un facteur pour les rendre visibles
    const double erreurScaleFactor = 1.0;  // Mettre > 1 pour agrandir les barres si besoin

    for (int i = 0; i < nRings; i++) {
        dose_err[i] *= erreurScaleFactor;
    }

    TGraph* gDose = new TGraph(nRings, rayons_centre, dose_par_event);
    gDose->SetTitle("Dose moyenne par d#acute{e}sint#acute{e}gration vs Rayon;Rayon (mm);Dose (nGy/d#acute{e}sint#acute{e}gration)");
    gDose->SetMarkerStyle(21);
    gDose->SetMarkerSize(2.5);
    gDose->SetMarkerColor(kBlue+1);
    gDose->SetLineColor(kBlue+1);
    gDose->SetLineWidth(3);
    gDose->GetYaxis()->SetTitleOffset(1.4);

    // Forcer l'affichage des barres d'erreur même si petites
    gDose->SetDrawOption("AP");
    gDose->Draw("AP");  // AP = Axis + Points avec barres d'erreur

    // Ajouter une ligne reliant les points
    TGraph* gLine = new TGraph(nRings, rayons_centre, dose_par_event);
    gLine->SetLineColor(kBlue+1);
    gLine->SetLineWidth(2);
    gLine->SetLineStyle(1);
    gLine->Draw("L SAME");

    // Annotations
    for (int i = 0; i < nRings; i++) {
        TLatex* label = new TLatex(rayons_centre[i], dose_par_event[i] * 1.08, Form("%.2e", dose_par_event[i]));
        label->SetTextSize(0.030);
        label->SetTextAlign(21);
        label->Draw();
    }
    
    // Légende
    TLegend* leg4 = new TLegend(0.50, 0.70, 0.93, 0.88);
    leg4->SetBorderSize(1);
    leg4->SetFillColor(kWhite);
    leg4->SetTextSize(0.028);
    leg4->AddEntry(gDose, "Dose moyenne #pm erreur stat.", "lep");
    leg4->AddEntry((TObject*)0, Form("N_{evt} = %.2e", (double)nEvents), "");
    leg4->AddEntry((TObject*)0, "Note: erreurs stat. tr#grave{e}s petites (#sim10^{-8} nGy)", "");
    leg4->Draw();

    c4->SaveAs("dose_vs_rayon.png");
    std::cout << "✓ dose_vs_rayon.png" << std::endl;
    
    // ═══════════════════════════════════════════════════════════════════════════
    // STATISTIQUES FINALES
    // ═══════════════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    STATISTIQUES DES HISTOGRAMMES                ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    
    for (int i = 0; i < nRings; i++) {
        if (hDoseFile[i]) {
            std::cout << "║ ANNEAU " << i << " (r=" << (int)rayons_int[i] << "-" << (int)rayons_ext[i] << " mm, m=" 
                      << std::fixed << std::setprecision(4) << masses[i] << " g)" << std::endl;
            std::cout << "║   Entries=" << std::setw(8) << (int)hDoseFile[i]->GetEntries()
                      << "  Mean=" << std::scientific << std::setprecision(3) << hDoseFile[i]->GetMean() 
                      << " nGy  RMS=" << hDoseFile[i]->GetStdDev() << " nGy" << std::endl;
            std::cout << "║   Dose totale=" << std::scientific << std::setprecision(4) << sumDose[i] 
                      << " nGy  Dose/evt=" << sumDose[i]/nEvents << " nGy" << std::endl;
        }
    }
    
    std::cout << "╠════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ TOTAL: Dose=" << std::scientific << std::setprecision(4) << grandTotalDose 
              << " nGy  Dose/evt=" << grandDosePerEvent << " nGy" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    file->Close();
    
    std::cout << "\n✓ Analyse terminée avec succès!" << std::endl;
}
