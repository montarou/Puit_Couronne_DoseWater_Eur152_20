#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <vector>
#include <set>

class EventAction;

/// @brief Suivi pas-à-pas des particules
///
/// Cette classe détecte les passages des particules dans les volumes
/// de détection (upstream, downstream, anneaux d'eau) et met à jour
/// les structures de EventAction.
///
/// Identification des primaires : parentID == 0

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction();
    
    virtual void UserSteppingAction(const G4Step*);
    
private:
    EventAction* fEventAction;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE DEBUG
    // ═══════════════════════════════════════════════════════════════
    G4bool fVerbose;
    G4int fVerboseMaxEvents;
    
    // ═══════════════════════════════════════════════════════════════
    // NOMS DES VOLUMES D'EAU (pour identification rapide)
    // ═══════════════════════════════════════════════════════════════
    std::set<G4String> fWaterRingNames;
};

#endif
