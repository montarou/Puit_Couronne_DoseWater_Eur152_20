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
   - Émission dans un cône de **20°** de demi-angle (optimisé pour irradier l'eau)
   - ~1.92 gammas par désintégration en moyenne
   - Activité : 44 kBq (sur 4π)

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

## Renormalisation Temporelle

### Principe

La simulation émet les gammas dans un cône restreint (20°) pour optimiser le temps de calcul en concentrant les particules vers le détecteur. Ceci nécessite une **renormalisation** pour convertir le nombre d'événements simulés en temps d'irradiation réel.

### Formules

**Fraction d'angle solide du cône :**
```
f = (1 - cos(θ)) / 2
```

Pour θ = 20° : **f = 0.0302 = 3.02%**

**Temps d'irradiation équivalent :**
```
T_irr = N_sim / (f × A)
```

Avec A = 44 kBq (activité sur 4π)

### Table de conversion (θ = 20°, A = 44 kBq)

| N_événements | Temps d'irradiation |
|--------------|---------------------|
| 1 000        | 0.75 s              |
| 10 000       | 7.5 s               |
| 100 000      | 75 s (1.25 min)     |
| 500 000      | 6.3 min             |
| 1 000 000    | 12.5 min            |
| 10 000 000   | 2.1 h               |

### Résumé affiché en fin de simulation

Le programme affiche automatiquement le temps d'irradiation équivalent :

```
╠═══════════════════════════════════════════════════════════════════╣
║  ★★★ RENORMALISATION SPATIALE ET TEMPORELLE ★★★                   ║
╟───────────────────────────────────────────────────────────────────╢
║  Paramètres de la source :                                        ║
║    Activité (4π)              : 44 kBq                            ║
║    Gammas moyens/désint.      : 1.924                             ║
╟───────────────────────────────────────────────────────────────────╢
║  Cône d'émission simulé :                                         ║
║    Demi-angle θ               : 20°                               ║
║    Angle solide Ω             : 0.379 sr                          ║
║    Fraction de 4π (f)         : 0.0302 (3.02%)                    ║
╟───────────────────────────────────────────────────────────────────╢
║  ══► TEMPS D'IRRADIATION ÉQUIVALENT :                             ║
║                                                                   ║
║         T_irr = 75.3 secondes                                     ║
║                                                                   ║
║    (soit 0.753 ms par événement simulé)                           ║
╠═══════════════════════════════════════════════════════════════════╣
```

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

### 1. Fichier de diagnostic : `output.log`

Le fichier `output.log` contient tous les messages de diagnostic détaillés :

```
╔═══════════════════════════════════════════════════════════════════╗
║            PUITS COURONNE - DIAGNOSTIC LOG                        ║
║            2025-01-15 14:30:00                                    ║
╚═══════════════════════════════════════════════════════════════════╝

BeginOfEvent 0 | 2 primary gamma(s) registered
UPSTREAM | Event 0 | PRIMARY gamma trackID=1 | E=344.28 keV
FILTER_ENTRY | Event 0 | trackID=1 | E=344.28 keV | z=37.5 mm
FILTER_EXIT | Event 0 | trackID=1 | E=344.28 keV | z=42.5 mm
WATER_ENTRY | Event 0 | gamma | trackID=1 | E=344.28 keV | r=5.2 mm | z=102.5 mm
WATER_DEPOSIT | Event 0 | Ring 1 | e- | edep=12.5 keV | r=7.3 mm | z=101.2 mm
...
==================================================
EVENT 0 SUMMARY
==================================================
Primary gammas: 2 | Total E: 465.06 keV
  [0] trackID=1 E_init=344.28 keV -> [TRANSMITTED]
  [1] trackID=2 E_init=120.78 keV -> [ABSORBED]
Dose dans les anneaux d'eau:
  Ring 1 (r=5-10 mm): 12.5 keV
  TOTAL: 12.5 keV
==================================================
```

**Types de messages :**
- `UPSTREAM` / `DOWNSTREAM` : passage aux plans de comptage
- `FILTER_ENTRY` / `FILTER_EXIT` : entrée/sortie du filtre W/PETG
- `CONTAINER_ENTRY` : entrée dans le container
- `WATER_ENTRY` : entrée dans l'eau (gamma ou électron)
- `WATER_DEPOSIT` : dépôt d'énergie dans un anneau
- `EVENT SUMMARY` : résumé de chaque événement

### 2. Fichier ROOT : `puits_couronne_output.root`

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
