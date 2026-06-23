#include "farm/ranch/RanchSystem.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/core/UnlockGraph.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cstdlib>

namespace farm {

RanchSystem::RanchSystem() {
    RanchFacilityData coop;
    coop.id = 1;
    coop.kind = RanchFacilityKind::ChickenCoop;
    coop.level = 1;
    coop.capacity = kChickenCoopBaseCapacity;
    facilities_.push_back(coop);
}

void RanchSystem::Setup(AchievementSystem* ach, DailyTaskSystem* dts) {
    ach_ = ach;
    dts_ = dts;
}

namespace {

bool FacilityMatches(RanchFacilityKind facility, AnimalKind animal) {
    return (facility == RanchFacilityKind::ChickenCoop && animal == AnimalKind::Chicken) ||
           (facility == RanchFacilityKind::CowBarn && animal == AnimalKind::Cow) ||
           (facility == RanchFacilityKind::SheepPen && animal == AnimalKind::Sheep);
}

int FacilityCapacity(RanchFacilityKind kind) {
    return kind == RanchFacilityKind::ChickenCoop ? kChickenCoopBaseCapacity : kBarnBaseCapacity;
}

int FacilityBuildCost(RanchFacilityKind kind) {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop: return 0;
        case RanchFacilityKind::CowBarn:     return 120;
        case RanchFacilityKind::SheepPen:    return 160;
        case RanchFacilityKind::PigPen:      return 160;
    }
    return 0;
}

int AnimalCost(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken: return kChickenCost;
        case AnimalKind::Cow:     return kCowCost;
        case AnimalKind::Sheep:   return kSheepCost;
        case AnimalKind::Pig:     return kSheepCost;
    }
    return kChickenCost;
}

ItemId FeedFor(AnimalKind kind) {
    return kind == AnimalKind::Chicken ? ItemId::ChickenFeed : ItemId::CowFeed;
}

ItemId ProductFor(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken: return ItemId::Egg;
        case AnimalKind::Cow:     return ItemId::Milk;
        case AnimalKind::Sheep:   return ItemId::Wool;
        case AnimalKind::Pig:     return ItemId::Wool;
    }
    return ItemId::Egg;
}

int ProductionTicksFor(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken: return kChickenEggTicks;
        case AnimalKind::Cow:     return kCowMilkTicks;
        case AnimalKind::Sheep:   return kSheepWoolTicks;
        case AnimalKind::Pig:     return kSheepWoolTicks;
    }
    return kChickenEggTicks;
}

int LifespanFor(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken: return kChickenLifespan;
        case AnimalKind::Cow:     return kCowLifespan;
        case AnimalKind::Sheep:   return kSheepLifespan;
        case AnimalKind::Pig:     return kSheepLifespan;
    }
    return kChickenLifespan;
}

}  // namespace

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
            if (animal.is_baby) continue;
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
    if (facility == nullptr) return {};
    std::vector<AnimalView> views;
    for (const AnimalData& animal : facility->animals) {
        const int remaining = animal.state == AnimalState::Producing
                                  ? std::max(0, animal.finish_tick - current_tick)
                                  : 0;
        views.push_back(AnimalView{animal.id, animal.kind, animal.state, remaining,
                                    animal.mood, animal.age_ticks,
                                    LifespanFor(animal.kind), animal.is_baby});
    }
    return views;
}

Result<int> RanchSystem::BuildFacility(PlayerState& player, RanchFacilityKind kind) {
    if (!player.IsUnlocked(UnlockGraph::FacilityUnlock(kind))) {
        return Result<int>::failure(ErrorCode::ContentLocked);
    }
    auto spent = player.TrySpendGold(FacilityBuildCost(kind));
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }
    RanchFacilityData facility;
    facility.id = next_facility_id_++;
    facility.kind = kind;
    facility.level = 1;
    facility.capacity = FacilityCapacity(kind);
    facilities_.push_back(facility);
    return Result<int>::success(facility.id);
}

Result<int> RanchSystem::BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr || !FacilityMatches(facility->kind, kind)) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    if (!player.IsUnlocked(UnlockGraph::AnimalUnlock(kind))) {
        return Result<int>::failure(ErrorCode::ContentLocked);
    }
    if (static_cast<int>(facility->animals.size()) >= facility->capacity) {
        return Result<int>::failure(ErrorCode::FacilityFull);
    }
    auto spent = player.TrySpendGold(AnimalCost(kind));
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }
    AnimalData animal;
    animal.id = next_animal_id_++;
    animal.kind = kind;
    animal.mood = kMoodFedValue;
    animal.last_fed_tick = 0;
    animal.age_ticks = 0;
    animal.is_baby = false;
    facility->animals.push_back(animal);
    if (ach_) ach_->OnBuyAnimal(kind);
    if (dts_) dts_->OnBuyAnimal(1);
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
    if (animal->is_baby) {
        return Result<void>::failure(ErrorCode::AnimalNotIdle);
    }
    if (animal->state != AnimalState::Idle) {
        return Result<void>::failure(ErrorCode::AnimalNotIdle);
    }
    auto removed = player.TryRemoveItem(FeedFor(animal->kind), 1);
    if (!removed.ok()) {
        return removed;
    }
    animal->mood = kMoodFedValue;
    animal->last_fed_tick = current_tick;

    int base_ticks = ProductionTicksFor(animal->kind);
    animal->state = AnimalState::Producing;
    animal->finish_tick = current_tick + base_ticks;
    if (dts_) dts_->OnFeedAnimal(1);
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
    auto added = player.TryAddItem(ProductFor(animal->kind), 1);
    if (!added.ok()) {
        return added;
    }
    animal->state = AnimalState::Idle;
    animal->finish_tick = 0;
    if (ach_) ach_->OnHarvestProduct(ProductFor(animal->kind));
    if (dts_) dts_->OnHarvestProduct(1);
    return Result<void>::success();
}

Result<int> RanchSystem::BatchFeed(PlayerState& player, int facility_id, int current_tick) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    int count = 0;
    for (AnimalData& animal : facility->animals) {
        if (animal.state != AnimalState::Idle || animal.is_baby ||
            !player.HasItem(FeedFor(animal.kind), 1)) {
            continue;
        }
        auto fed = FeedAnimal(player, facility_id, animal.id, current_tick);
        if (fed.ok()) ++count;
    }
    return Result<int>::success(count);
}

int RanchSystem::ReadyProductCount() const {
    int count = 0;
    for (const RanchFacilityData& facility : facilities_) {
        for (const AnimalData& animal : facility.animals) {
            if (animal.state == AnimalState::Ready) ++count;
        }
    }
    return count;
}

Result<int> RanchSystem::BatchHarvest(PlayerState& player, int facility_id) {
    RanchFacilityData* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::FacilityOutOfRange);
    }
    int count = 0;
    for (AnimalData& animal : facility->animals) {
        if (animal.state != AnimalState::Ready) continue;
        auto harvested = HarvestAnimal(player, facility_id, animal.id);
        if (harvested.ok()) ++count;
        else if (harvested.code == ErrorCode::WarehouseFull) break;
    }
    return Result<int>::success(count);
}

void RanchSystem::Tick(int current_tick, float weather_multiplier) {
    for (RanchFacilityData& facility : facilities_) {
        std::vector<int> dead_ids;
        std::vector<AnimalData> new_babies;

        for (AnimalData& animal : facility.animals) {
            ++animal.age_ticks;

            if (animal.age_ticks >= LifespanFor(animal.kind)) {
                dead_ids.push_back(animal.id);
                continue;
            }

            if (animal.is_baby && animal.age_ticks >= kBabyGrowTicks) {
                animal.is_baby = false;
                animal.mood = kMoodFedValue;
                animal.last_fed_tick = current_tick;
            }
            if (animal.is_baby) continue;

            if (current_tick - animal.last_fed_tick > kMoodDropAfterTicks) {
                animal.mood = std::max(0, animal.mood - kMoodDropPerTick);
            }

            if (animal.state == AnimalState::Producing) {
                int effective_tick = animal.finish_tick;
                if (animal.mood >= kMoodHighThreshold) {
                    if (current_tick % 5 == 0) --effective_tick;
                } else if (animal.mood < kMoodLowThreshold) {
                    if (current_tick % 3 == 0) ++effective_tick;
                }
                if (current_tick >= effective_tick) {
                    animal.state = AnimalState::Ready;
                } else {
                    animal.finish_tick = effective_tick;
                }
            } else if (animal.state == AnimalState::Producing &&
                       current_tick >= animal.finish_tick) {
                animal.state = AnimalState::Ready;
            }
        }

        for (int dead_id : dead_ids) {
            facility.animals.erase(
                std::remove_if(facility.animals.begin(), facility.animals.end(),
                               [dead_id](const AnimalData& a) { return a.id == dead_id; }),
                facility.animals.end());
        }

        if (current_tick % kBreedingIntervalTicks == 0) {
            for (std::size_t i = 0; i < facility.animals.size(); ++i) {
                for (std::size_t j = i + 1; j < facility.animals.size(); ++j) {
                    if (facility.animals[i].kind == facility.animals[j].kind &&
                        !facility.animals[i].is_baby && !facility.animals[j].is_baby &&
                        facility.animals[i].mood >= kMoodHighThreshold &&
                        facility.animals[j].mood >= kMoodHighThreshold &&
                        static_cast<int>(facility.animals.size() + new_babies.size()) <
                            facility.capacity) {
                        if ((current_tick + static_cast<int>(i)) % 100 < kBreedingChancePercent) {
                            AnimalData baby;
                            baby.id = next_animal_id_++;
                            baby.kind = facility.animals[i].kind;
                            baby.is_baby = true;
                            baby.age_ticks = 0;
                            baby.mood = kMoodFedValue;
                            baby.last_fed_tick = current_tick;
                            new_babies.push_back(baby);
                            break;
                        }
                    }
                }
                if (!new_babies.empty()) break;
            }
        }

        for (AnimalData& baby : new_babies) {
            facility.animals.push_back(baby);
        }
    }
}

void RanchSystem::ClearForLoad() {
    facilities_.clear();
    next_animal_id_ = 1;
    next_facility_id_ = 1;
}

void RanchSystem::SetFacilitiesForLoad(const std::vector<RanchFacilityData>& facilities,
                                       int next_animal_id) {
    facilities_ = facilities;
    next_animal_id_ = std::max(1, next_animal_id);
    next_facility_id_ = 1;
    for (const RanchFacilityData& facility : facilities_) {
        next_facility_id_ = std::max(next_facility_id_, facility.id + 1);
    }
}

RanchFacilityData* RanchSystem::FindFacility(int facility_id) {
    for (RanchFacilityData& facility : facilities_) {
        if (facility.id == facility_id) return &facility;
    }
    return nullptr;
}

const RanchFacilityData* RanchSystem::FindFacility(int facility_id) const {
    for (const RanchFacilityData& facility : facilities_) {
        if (facility.id == facility_id) return &facility;
    }
    return nullptr;
}

AnimalData* RanchSystem::FindAnimal(RanchFacilityData& facility, int animal_id) {
    for (AnimalData& animal : facility.animals) {
        if (animal.id == animal_id) return &animal;
    }
    return nullptr;
}

const AnimalData* RanchSystem::FindAnimal(const RanchFacilityData& facility, int animal_id) const {
    for (const AnimalData& animal : facility.animals) {
        if (animal.id == animal_id) return &animal;
    }
    return nullptr;
}

}  // namespace farm
