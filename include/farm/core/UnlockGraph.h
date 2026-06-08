#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

struct UnlockNode {
    UnlockId id = UnlockId::WheatSeed;
    UnlockCategory category = UnlockCategory::Seed;
    const char* name = "";
    int required_level = 1;
    int gold_cost = 0;
    std::vector<UnlockId> prerequisites;
};

class UnlockGraph {
public:
    static const std::vector<UnlockNode>& Nodes();
    static const UnlockNode* Find(UnlockId id);
    static UnlockId SeedUnlock(ItemId seed);
    static UnlockId FacilityUnlock(RanchFacilityKind kind);
    static UnlockId AnimalUnlock(AnimalKind kind);
    static bool IsSeed(ItemId item);
};

}  // namespace farm

