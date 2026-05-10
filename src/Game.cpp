#include "farm/Game.h"

#include <ostream>

namespace farm {

// Advances the global simulation clock by exactly one tick.
//
// Contract:
// - current_tick_ is incremented first, then every subsystem receives that new tick value.
// - Order of Tick calls (fixed):
//   1) PlantingSystem — fields / crops
//   2) RanchSystem — chicken coop (and future livestock)
//   3) WorkshopSystem — feed mill (and future workshops)
//   4) OrderSystem — order-board timers (currently unused in V1)
//
// Why this order is OK today:
// - No subsystem reads another subsystem's post-Tick state in the same AdvanceTick(); each only
//   updates its own domain using PlayerState when the player acts.
// - If you add cross-domain effects in one tick (e.g. workshop completion feeding ranch), define
//   an explicit rule: either resolve in player-triggered APIs, or document a new Tick order and
//   add integration tests.
//
// Future extensions: append new Tick hooks in a documented position; avoid hidden time bases.

void Game::AdvanceTick() {
    ++current_tick_;
    planting_.Tick(current_tick_);
    ranch_.Tick(current_tick_);
    workshop_.Tick(current_tick_);
    orders_.Tick(current_tick_);
}

void Game::WriteStatusSummary(std::ostream& os) const {
    os << "tick=" << current_tick_ << " gold=" << player_.Gold() << "\n";
    orders_.WriteBoardForDisplay(os);
    workshop_.WriteStatus(os);
    ranch_.WriteStatus(os);
}

}  // namespace farm
