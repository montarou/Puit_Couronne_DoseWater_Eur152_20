# Puits Couronne - Simulation Geant4

## Description

Simulation Geant4 pour mesurer la dose déposée dans un détecteur d'eau segmenté en anneaux concentriques, placé dans un container W/PETG en forme de "puits couronne".

## Géométrie

```
                    z (cm)
                    ↑
                    │
                    │   ┌─────────────┐
                    │   │  Fond W/PETG │ z = 10.55 cm
           10.0 ────┼───├─────────────┤
                    │   │   EAU       │ z = 10.0 - 10.25 cm (5 anneaux)
                    │   │  (anneaux)  │
                    │   └─────────────┘ z = 9.75 cm (ouvert)
                    │   │ Paroi W/PETG│
                    │
                    │
            4.0 ────┼───┌─────────────┐
                    │   │ Filtre      │ z = 3.75 - 4.25 cm
                    │   │ W/PETG      │
                    │   └─────────────┘
                    │
            2.0 ────┼───────X──────────  Source Eu-152
                    │
                    └───────────────────→ x, y
```

### Composants

1. **Source Europium-152** (z = 2 cm)
   - Spectre gamma réaliste (12 raies principales)
   - Émission dans un cône de 60° de demi-angle
   - ~1.92 gammas par désintégration en moyenne

2. **Filtre W/PETG** (z = 4 cm)
   - Diamètre : 5 cm
   - Épaisseur : 5 mm
   - Composition : 75% W / 25% PETG en masse

3. **Container "Puits Couronne"** (z = 10 cm)
   - Matériau : W/PETG (75%/25%)
   - Rayon intérieur : 2.5 cm
   - Hauteur intérieure : 7 mm
   - Épaisseur parois : 2 mm
   - Face inférieure **ouverte** (vers la source)

4. **Détecteur Eau** (à l'intérieur du container)
   - Épaisseur : 5 mm
   - Position : contre le fond supérieur interne
   - **Segmentation en 5 anneaux concentriques** :

   | Anneau | R_in (mm) | R_out (mm) |
   |--------|-----------|------------|
   | 0      | 0         | 5          |
   | 1      | 5         | 10         |
   | 2      | 10        | 15         |
   | 3      | 15        | 20         |
   | 4      | 20        | 25         |

## Compilation

```bash
mkdir build
cd build
cmake ..
make -j4
```

## Utilisation

### Mode interactif (avec visualisation)
```bash
./puits_couronne
```

### Mode batch
```bash
./puits_couronne run.mac
```

## Fichiers de sortie

Le fichier `puits_couronne_output.root` contient :

### Ntuples

1. **EventData** : Données par événement
   - eventID, nPrimaries, totalEnergy
   - nTransmitted, nAbsorbed, nScattered
   - totalWaterDeposit

2. **GammaData** : Données par gamma primaire
   - eventID, gammaIndex
   - energyInitial, energyUpstream, energyDownstream
   - theta, phi
   - detectedUpstream, detectedDownstream, transmitted

3. **RingDoseData** : **Dose par anneau par désintégration**
   - eventID, nPrimaries
   - doseRing0, doseRing1, doseRing2, doseRing3, doseRing4 (en keV)
   - doseTotal

### Histogrammes

- H0: nGammasPerEvent
- H1: energySpectrum
- H2: totalEnergyPerEvent
- H3-H7: doseRing0 à doseRing4
- H8: doseTotalWater

## Analyse des résultats

Le ntuple `RingDoseData` permet d'analyser la dose **désintégration par désintégration** dans chaque anneau. Exemple d'analyse ROOT :

```cpp
TFile* f = TFile::Open("puits_couronne_output.root");
TTree* t = (TTree*)f->Get("RingDoseData");

// Distribution de dose dans l'anneau central
t->Draw("doseRing0");

// Corrélation entre anneaux
t->Draw("doseRing0:doseRing4", "doseRing0>0 && doseRing4>0");

// Dose totale quand nPrimaries > 0
t->Draw("doseTotal", "nPrimaries>0 && doseTotal>0");
```

## Physique

- Liste de physique : FTFP_BERT
- Modèles EM : Livermore (optimisés basse énergie)
- Cuts de production : 0.1 mm
- Step limiter activé dans les volumes d'eau

## Auteur

Simulation créée pour l'étude de la dose dans un détecteur liquide avec blindage W/PETG.
