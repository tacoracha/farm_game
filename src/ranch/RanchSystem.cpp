#include "farm/ranch/RanchSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>

namespace farm {

RanchSystem::RanchSystem() {
    RanchFacilityData coop;
    coop.id = 1;
    coop.kind = RanchFacilityKind::ChickenCoop;
    coop.level = 1;
    coop.capacity = kChickenCoopBaseCapacity;
    facilities_.push_back(coop);
}

std::vector<RanchFacilityView> RanchSystem::FacilityViews() const {
    std::vector<RanchFacilityView> views;
    for (const RanchFacilityData& facility : facilities_) {
        RanchFacilityView view;
        view.id = facility.id;
        view.kind = facility.kind;
        view.level = facility.level;
        view.capacity = facility.capacity;
        view.animal_count = static_cast<int>(facility.animals.size());
        for (const AnimalData& animal : facility.animals) {
            if (animal.state == AnimalState::Idle) {
                ++view.idle_count;
            } else if (animal.state == AnimalState::Producing) {
                ++view.producing_count;
            } else if (animal.state == AnimalState::Ready) {
                ++view.ready_count;
            }
        }
        views.push_back(view);
    }
    return views;
}

std::vector<AnimalView> RanchSystem::AnimalViews(int facility_id, int current_tick) const {
    const RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return {};
    }
    std::vector<AnimalView> views;
    for (const AnimalData& animal : facility->animals) {
        const int remaining = animal.state == AnimalState::Producing
                                  ? std::max(0, animal.finish_tick - current_tick)
                                  : 0;
        views.push_back(AnimalView{animal.id, animal.state, remaining});
    }
    return views;
}

Result<int> RanchSystem::BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr || facility->kind != RanchFacilityKind::ChickenCoop ||
        kind != AnimalKind::Chicken) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    if (static_cast<int>(facility->animals.size()) >= facility->capacity) {
        return Result<int>::failure(ErrorCode::FacilityFull);
    }
    auto spent = player.TrySpendGold(kChickenCost);
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }
    AnimalData animal;
    animal.id = next_animal_id_++;
    animal.kind = kind;
    facility->animals.push_back(animal);
    return Result<int>::success(animal.id);
}

Result<void> RanchSystem::FeedAnimal(PlayerState& player, int facility_id, int animal_id,
                                     int current_tick) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<void>::failure(ErrorCode::FacilityOutOfRange);
    }
    AnimalData* animal = FindAnimal(*facility, animal_id);
    if (animal == nullptr) {
        return Result<void>::failure(ErrorCode::AnimalOutOfRange);
    }
    if (animal->state != AnimalState::Idle) {
        return Result<void>::failure(ErrorCode::AnimalNotIdle);
    }
    auto removed = player.TryRemoveItem(ItemId::ChickenFeed, 1);
    if (!removed.ok()) {
        return removed;
    }
    animal->state = AnimalState::Producing;
    animal->finish_tick = current_tick + kChickenEggTicks;
    return Result<void>::success();
}

Result<void> RanchSystem::HarvestAnimal(PlayerState& player, int facility_id, int animal_id) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<void>::failure(ErrorCode::FacilityOutOfRange);
    }
    AnimalData* animal = FindAnimal(*facility, animal_id);
    if (animal == nullptr) {
        return Result<void>::failure(ErrorCode::AnimalOutOfRange);
    }
    if (animal->state != AnimalState::Ready) {
        return Result<void>::failure(ErrorCode::AnimalNotReady);
    }
    auto added = player.TryAddItem(ItemId::Egg, 1);
    if (!added.ok()) {
        return added;
    }
    animal->state = AnimalState::Idle;
    animal->finish_tick = 0;
    return Result<void>::success();
}

Result<int> RanchSystem::BatchFeed(PlayerState& player, int facility_id, int current_tick) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    int count = 0;
    for (AnimalData& animal : facility->animals) {
        if (animal.state != AnimalState::Idle || !player.HasItem(ItemId::ChickenFeed, 1)) {
            continue;
        }
        auto fed = FeedAnimal(player, facility_id, animal.id, current_tick);
        if (fed.ok()) {
            ++count;
        }
    }
    return Result<int>::success(count);
}

Result<int> RanchSystem::BatchHarvest(PlayerState& player, int facility_id) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    int count = 0;
    for (AnimalData& animal : facility->animals) {
        if (animal.state != AnimalState::Ready) {
            continue;
        }
        auto harvested = HarvestAnimal(player, facility_id, animal.id);
        if (harvested.ok()) {
            ++count;
        } else if (harvested.code == ErrorCode::WarehouseFull) {
            break;
        }
    }
    return Result<int>::success(count);
}

void RanchSystem::Tick(int current_tick, float weather_multiplier) {
    const int bonus = weather_multiplier > 1.0f ? 1 : 0;
    for (RanchFacilityData& facility : facilities_) {
        for (AnimalData& animal : facility.animals) {
            if (animal.state == AnimalState::Producing &&
                current_tick + bonus >= animal.finish_tick) {
                animal.state = AnimalState::Ready;
            }
        }
    }
}

void RanchSystem::ClearForLoad() {
    facilities_.clear();
    next_animal_id_ = 1;
}

void RanchSystem::SetFacilitiesForLoad(const std::vector<RanchFacilityData>& facilities,
                                       int next_animal_id) {
    facilities_ = facilities;
    next_animal_id_ = std::max(1, next_animal_id);
}

RanchFacilityData* RanchSystem::FindFacility(int facility_id) {
    for (RanchFacilityData& facility : facilities_) {
        if (facility.id == facility_id) {
            return &facility;
        }
    }
    return nullptr;
}

const RanchFacilityData* RanchSystem::FindFacility(int facility_id) const {
    for (const RanchFacilityData& facility : facilities_) {
        if (facility.id == facility_id) {
            return &facility;
        }
    }
    return nullptr;
}

AnimalData* RanchSystem::FindAnimal(RanchFacilityData& facility, int animal_id) {
    for (AnimalData& animal : facility.animals) {
        if (animal.id == animal_id) {
            return &animal;
        }
    }
    return nullptr;
}

const AnimalData* RanchSystem::FindAnimal(const RanchFacilityData& facility, int animal_id) const {
    for (const AnimalData& animal : facility.animals) {
        if (animal.id == animal_id) {
            return &animal;
        }
    }
    return nullptr;
}

}  // namespace farm

