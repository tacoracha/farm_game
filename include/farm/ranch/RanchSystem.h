#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct AnimalData {
    int id = 0;
    AnimalKind kind = AnimalKind::Chicken;
    AnimalQuality quality = AnimalQuality::Common;
    AnimalState state = AnimalState::Idle;
    int finish_tick = 0;
};

struct RanchFacilityData {
    int id = 1;
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
    void Tick(int current_tick, float weather_multiplier);

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
};

}  // namespace farm
