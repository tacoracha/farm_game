#pragma once

#include "OrderSystem.h"
#include "PlantingSystem.h"
#include "PlayerState.h"
#include "RanchSystem.h"
#include "ShopSystem.h"
#include "WorkshopSystem.h"

#include <iosfwd>

namespace farm {

class Game {
public:
    Game() = default;

    int CurrentTick() const { return current_tick_; }

    PlayerState& Player() { return player_; }
    const PlayerState& Player() const { return player_; }

    PlantingSystem& Planting() { return planting_; }
    const PlantingSystem& Planting() const { return planting_; }

    ShopSystem& Shop() { return shop_; }
    RanchSystem& Ranch() { return ranch_; }
    const RanchSystem& Ranch() const { return ranch_; }
    WorkshopSystem& Workshop() { return workshop_; }
    const WorkshopSystem& Workshop() const { return workshop_; }
    OrderSystem& Orders() { return orders_; }
    const OrderSystem& Orders() const { return orders_; }

    void AdvanceTick();

    void WriteStatusSummary(std::ostream& os) const;

private:
    int current_tick_ = 0;
    PlayerState player_;
    PlantingSystem planting_;
    ShopSystem shop_;
    RanchSystem ranch_;
    WorkshopSystem workshop_;
    OrderSystem orders_;
};

}  // namespace farm
