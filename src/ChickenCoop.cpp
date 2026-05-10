#include "farm/ChickenCoop.h"

#include "farm/PlayerState.h"

#include <ostream>

namespace farm {

ChickenCoop::ChickenCoop() : slots_(static_cast<std::size_t>(kChickenCoopSlotCount)) {}

AnimalState ChickenCoop::GetState(int slot_index) const {
    if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size())) {
        return AnimalState::Idle;
    }
    return slots_[static_cast<std::size_t>(slot_index)].state;
}

Result<void> ChickenCoop::FeedChicken(PlayerState& player, int slot_index, int current_tick) {
    if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size())) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }

    ChickenSlot& slot = slots_[static_cast<std::size_t>(slot_index)];
    if (slot.state != AnimalState::Idle) {
        return Result<void>::failure(ErrorCode::ChickenNotIdleForFeed);
    }

    auto removed = player.TryRemoveFromWarehouse(ItemId::ChickenFeed, 1);
    if (!removed.ok()) {
        return removed;
    }

    slot.state = AnimalState::Producing;
    slot.finish_tick = current_tick + kChickenEggProductionTicks;
    return Result<void>::success();
}

void ChickenCoop::Tick(int current_tick) {
    for (ChickenSlot& slot : slots_) {
        if (slot.state == AnimalState::Producing && current_tick >= slot.finish_tick) {
            slot.state = AnimalState::Ready;
        }
    }
}

Result<void> ChickenCoop::CollectEgg(PlayerState& player, int slot_index) {
    if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size())) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }

    ChickenSlot& slot = slots_[static_cast<std::size_t>(slot_index)];
    if (slot.state != AnimalState::Ready) {
        return Result<void>::failure(ErrorCode::ChickenNotReadyToCollectEgg);
    }

    auto added = player.TryAddToWarehouse(ItemId::Egg, 1);
    if (!added.ok()) {
        return added;
    }

    slot.state = AnimalState::Idle;
    slot.finish_tick = 0;
    return Result<void>::success();
}

void ChickenCoop::WriteStatus(std::ostream& os) const {
    os << "ChickenCoop (" << static_cast<int>(slots_.size()) << " slots):\n";
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const ChickenSlot& s = slots_[i];
        os << "  [" << i << "] ";
        switch (s.state) {
            case AnimalState::Idle:
                os << "Idle\n";
                break;
            case AnimalState::Producing:
                os << "Producing egg until tick " << s.finish_tick << "\n";
                break;
            case AnimalState::Ready:
                os << "Ready (egg)\n";
                break;
        }
    }
}

}  // namespace farm
