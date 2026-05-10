#include "farm/WorkshopSystem.h"

#include <ostream>

namespace farm {

void WorkshopSystem::Tick(int current_tick) { feed_mill_.Tick(current_tick); }

void WorkshopSystem::WriteStatus(std::ostream& os) const { feed_mill_.WriteStatus(os); }

}  // namespace farm
