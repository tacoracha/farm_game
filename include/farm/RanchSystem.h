#pragma once

#include "ChickenCoop.h"

#include <iosfwd>

namespace farm {

// V1: chicken coop only; future species/buildings can extend here.
class RanchSystem {
public:
    void Tick(int current_tick);

    ChickenCoop& GetChickenCoop() { return chicken_coop_; }
    const ChickenCoop& GetChickenCoop() const { return chicken_coop_; }

    void WriteStatus(std::ostream& os) const;

private:
    ChickenCoop chicken_coop_;
};

}  // namespace farm
