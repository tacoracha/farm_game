#include "farm/RanchSystem.h"

#include <ostream>

namespace farm {

void RanchSystem::Tick(int current_tick) { chicken_coop_.Tick(current_tick); }

void RanchSystem::WriteStatus(std::ostream& os) const { chicken_coop_.WriteStatus(os); }

}  // namespace farm
