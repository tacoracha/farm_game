#pragma once

#include "farm/common/Types.h"
#include "farm/core/FarmEntity.h"

#include <vector>

namespace farm {

class PlayerState;
class AchievementSystem;
class DailyTaskSystem;

struct AnimalData final : public RanchEntity {
    RanchEntityType RanchType() const noexcept override { return RanchEntityType::Animal; }
    bool HasReadyOutput() const noexcept override { return state == AnimalState::Ready; }

    AnimalKind kind = AnimalKind::Chicken;
    AnimalQuality quality = AnimalQuality::Common;
    AnimalState state = AnimalState::Idle;
    int finish_tick = 0;
    int mood = 100;           // 0-100, affects production speed
    int last_fed_tick = 0;   // when the animal was last fed
    int age_ticks = 0;       // total ticks alive
    bool is_baby = false;    // juvenile, not producing yet
};

struct RanchFacilityData final : public RanchEntity {
    RanchFacilityData() { id = 1; }

    RanchEntityType RanchType() const noexcept override { return RanchEntityType::Facility; }
    bool HasReadyOutput() const noexcept override {
        for (const AnimalData& animal : animals) {
            if (animal.HasReadyOutput()) return true;
        }
        return false;
    }

    RanchFacilityKind kind = RanchFacilityKind::ChickenCoop;
    int level = 1;
    int capacity = 0;
    std::vector<AnimalData> animals;
};

struct AnimalView {
    int id = 0;
    AnimalKind kind = AnimalKind::Chicken;
    AnimalState state = AnimalState::Idle;
    int remaining_ticks = 0;
    int mood = 100;
    int age_ticks = 0;
    int max_age = 0;
    bool is_baby = false;
};

struct RanchFacilityView {
    int id = 1;
    RanchFacilityKind kind = RanchFacilityKind::ChickenCoop;
    int level = 1;
    int capacity = 0;
    int animal_count = 0;
    int idle_count = 0;
    int producing_count = 0;
    int ready_count = 0;
};

class RanchSystem {
public:
    RanchSystem();

    const std::vector<RanchFacilityData>& Facilities() const { return facilities_; }
    std::vector<RanchFacilityView> FacilityViews() const;
    std::vector<AnimalView> AnimalViews(int facility_id, int current_tick) const;

    Result<int> BuildFacility(PlayerState& player, RanchFacilityKind kind);
    Result<int> BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind);
    Result<void> FeedAnimal(PlayerState& player, int facility_id, int animal_id, int current_tick);
    Result<void> HarvestAnimal(PlayerState& player, int facility_id, int animal_id);
    Result<int> BatchFeed(PlayerState& player, int facility_id, int current_tick);
    Result<int> BatchHarvest(PlayerState& player, int facility_id);
    int ReadyProductCount() const;
    void Tick(int current_tick, float weather_multiplier);

    // Called by Game after construction to wire up event systems
    void Setup(AchievementSystem* ach, DailyTaskSystem* dts);

    // Save/Load — called by SaveManager via Game
    void ClearForLoad();
    void SetFacilitiesForLoad(const std::vector<RanchFacilityData>& facilities, int next_animal_id);
    int NextAnimalIdForSave() const { return next_animal_id_; }
    int NextFacilityIdForSave() const { return next_facility_id_; }

private:
    RanchFacilityData* FindFacility(int facility_id);
    const RanchFacilityData* FindFacility(int facility_id) const;
    AnimalData* FindAnimal(RanchFacilityData& facility, int animal_id);
    const AnimalData* FindAnimal(const RanchFacilityData& facility, int animal_id) const;

    std::vector<RanchFacilityData> facilities_;
    int next_animal_id_ = 1;
    int next_facility_id_ = 2;

    // Event system pointers (non-owning)
    AchievementSystem* ach_ = nullptr;
    DailyTaskSystem* dts_ = nullptr;
};

}  // namespace farm
