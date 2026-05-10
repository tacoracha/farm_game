#pragma once

#include "Constants.h"
#include "Types.h"

#include <iosfwd>
#include <vector>

namespace farm {

class PlayerState;

enum class AnimalState : std::uint8_t {
    Idle,
    Producing,
    Ready,
};

struct ChickenSlot {
    AnimalState state = AnimalState::Idle;
    int finish_tick = 0;
};

class ChickenCoop {
public:
    ChickenCoop();

    [[nodiscard]] int GetSlotCount() const { return static_cast<int>(slots_.size()); }

    [[nodiscard]] AnimalState GetState(int slot_index) const;

    [[nodiscard]] Result<void> FeedChicken(PlayerState& player, int slot_index, int current_tick);

    void Tick(int current_tick);

    [[nodiscard]] Result<void> CollectEgg(PlayerState& player, int slot_index);

    void WriteStatus(std::ostream& os) const;

private:
    std::vector<ChickenSlot> slots_;
};

}  // namespace farm
