#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr),
  fLastEventGammaCount(0),
  fConeAngle(20.*deg),
  fSourcePosition(0., 0., 2.*cm)
{
    // Créer le particle gun
    fParticleGun = new G4ParticleGun(1);
    
    // Configuration par défaut : gamma
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticlePosition(fSourcePosition);
    
    // ═══════════════════════════════════════════════════════════════
    // SPECTRE GAMMA EUROPIUM-152
    // ═══════════════════════════════════════════════════════════════
    // Source: NNDC/ENSDF - principales raies gamma
    // Énergies en keV, intensités en % par désintégration
    
    // Raies principales (intensité > 2%)
    fGammaEnergies = {
        121.78,   // Intensité: 28.41%
        244.70,   // Intensité: 7.53%
        344.28,   // Intensité: 26.59%
        411.12,   // Intensité: 2.24%
        443.97,   // Intensité: 2.83%
        778.90,   // Intensité: 12.97%
        867.38,   // Intensité: 4.24%
        964.08,   // Intensité: 14.63%
        1085.87,  // Intensité: 10.21%
        1112.07,  // Intensité: 13.64%
        1408.01   // Intensité: 21.01%
    };
    
    fGammaIntensities = {
        28.41,
        7.53,
        26.59,
        2.24,
        2.83,
        12.97,
        4.24,
        14.63,
        10.21,
        13.64,
        21.01
    };
    
    // Calculer les probabilités d'émission (normalisées pour le sampling)
    // Note: la somme des intensités est ~144.3%, donc en moyenne 1.443 gamma/désintégration
    // pour ces raies principales. Le total réel est ~192.4% (~1.924 gamma/désint.)
    
    G4double totalIntensity = 0.;
    for (const auto& intensity : fGammaIntensities) {
        totalIntensity += intensity;
    }
    
    // Pour chaque raie, probabilité = intensité / 100 (car on génère tous les gammas)
    fGammaProbabilities.resize(fGammaEnergies.size());
    for (size_t i = 0; i < fGammaIntensities.size(); ++i) {
        fGammaProbabilities[i] = fGammaIntensities[i] / 100.;
    }
    
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  PrimaryGeneratorAction: Spectre Eu-152 initialisé            ║" << G4endl;
    G4cout << "║  " << fGammaEnergies.size() << " raies gamma principales                                   ║" << G4endl;
    G4cout << "║  Intensité totale: " << totalIntensity << "%                                   ║" << G4endl;
    G4cout << "║  Gammas moyens/désintégration: ~" << totalIntensity/100. << "                         ║" << G4endl;
    G4cout << "║  Angle du cône: " << fConeAngle/deg << " degrés                                    ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    fLastEventGammaCount = 0;
    
    // Pour chaque raie gamma, tirer si elle est émise
    for (size_t i = 0; i < fGammaEnergies.size(); ++i) {
        G4double random = G4UniformRand();
        
        if (random < fGammaProbabilities[i]) {
            // Cette raie est émise
            
            // Énergie de la raie
            G4double energy = fGammaEnergies[i] * keV;
            
            // Générer une direction dans le cône
            G4double theta, phi;
            G4ThreeVector direction;
            GenerateDirectionInCone(fConeAngle, theta, phi, direction);
            
            // Configurer et tirer
            fParticleGun->SetParticleEnergy(energy);
            fParticleGun->SetParticleMomentumDirection(direction);
            fParticleGun->SetParticlePosition(fSourcePosition);
            fParticleGun->GeneratePrimaryVertex(anEvent);
            
            fLastEventGammaCount++;
        }
    }
    
    // Si aucun gamma n'a été émis, on peut quand même avoir un événement "vide"
    // C'est physiquement correct car certaines désintégrations peuvent ne pas
    // émettre de gamma dans le cône d'émission
}

void PrimaryGeneratorAction::GenerateDirectionInCone(G4double coneAngle,
                                                     G4double& theta,
                                                     G4double& phi,
                                                     G4ThreeVector& direction)
{
    // Distribution uniforme sur la surface de la calotte sphérique
    // cos(theta) est uniforme entre cos(coneAngle) et 1
    G4double cosTheta = 1. - G4UniformRand() * (1. - std::cos(coneAngle));
    theta = std::acos(cosTheta);
    
    // phi est uniforme entre 0 et 2π
    phi = G4UniformRand() * 2. * CLHEP::pi;
    
    // Calculer la direction
    G4double sinTheta = std::sin(theta);
    direction.set(sinTheta * std::cos(phi),
                  sinTheta * std::sin(phi),
                  cosTheta);
}
