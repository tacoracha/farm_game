#pragma once

#include "ChickenCoop.h"
#include "Types.h"

#include <iosfwd>
#include <vector>

namespace farm {

class PlayerState;

enum class AnimalKind : std::uint8_t {
    Chicken,
    Cow,
    Pig,
    Sheep,
};

enum class AnimalQuality : std::uint8_t {
    Common,
    Fine,
    Rare,
};

enum class RanchFacilityKind : std::uint8_t {
    ChickenCoop,
    CowBarn,
    PigPen,
    SheepPen,
};

struct AnimalView {
    int animal_id = -1;
    AnimalKind kind = AnimalKind::Chicken;
    AnimalQuality quality = AnimalQuality::Common;
    AnimalState state = AnimalState::Idle;
    int finish_tick = 0;
    int remaining_ticks = 0;
    bool fed = false;
    ItemId product_id = ItemId::Egg;
};

struct RanchFacilityView {
    int facility_id = -1;
    RanchFacilityKind kind = RanchFacilityKind::ChickenCoop;
    int level = 1;
    int capacity = 0;
    int idle_count = 0;
    int producing_count = 0;
    int harvestable_count = 0;
    int proficiency_exp = 0;
};

struct AnimalQualityRule {
    AnimalQuality quality = AnimalQuality::Common;
    int spawn_weight = 0;
    int output_bonus_percent = 0;
    int exp_bonus_percent = 0;
    int order_value_bonus_percent = 0;
};

struct RanchUpgradeCost {
    int gold = 0;
    ItemId tool_id = ItemId::Fertilizer;
    int tool_count = 0;
};

struct RanchFacilityLevelRule {
    RanchFacilityKind kind = RanchFacilityKind::ChickenCoop;
    int level = 1;
    int animal_capacity = 0;
    int production_speed_bonus_percent = 0;
    int fine_quality_unlock_level = 2;
    int rare_quality_unlock_level = 4;
    bool batch_feed_unlocked = false;
    bool batch_harvest_unlocked = false;
    bool auto_harvest_unlocked = false;
    RanchUpgradeCost next_upgrade_cost{};
};

class RanchSystem {
public:
    RanchSystem();

    void Init();
    void Tick(int current_tick);
    void OnTick(int tick_delta, float weather_multiplier);

    ChickenCoop& GetChickenCoop() { return chicken_coop_; }
    const ChickenCoop& GetChickenCoop() const { return chicken_coop_; }

    [[nodiscard]] std::vector<RanchFacilityView> GetAllFacilities() const;
    [[nodiscard]] RanchFacilityView GetFacilityView(int facility_id) const;
    [[nodiscard]] std::vector<AnimalView> GetAnimals(int facility_id) const;

    [[nodiscard]] Result<void> BuildFacility(PlayerState& player, RanchFacilityKind kind);
    [[nodiscard]] Result<void> UpgradeFacility(PlayerState& player, int facility_id);
    [[nodiscard]] bool IsRanchUnlocked(RanchFacilityKind kind, int player_level) const;

    [[nodiscard]] Result<int> BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind,
                                        AnimalQuality quality);
    [[nodiscard]] Result<void> FeedAnimal(PlayerState& player, int facility_id, int animal_id,
                                          int current_tick);
    [[nodiscard]] Result<void> HarvestAnimal(PlayerState& player, int facility_id, int animal_id);
    [[nodiscard]] Result<int> BatchFeed(PlayerState& player, int facility_id, int current_tick);
    [[nodiscard]] Result<int> BatchHarvest(PlayerState& player, int facility_id);

    [[nodiscard]] bool IsAnimalProductUnlocked(ItemId product_id) const;
    [[nodiscard]] int EstimateDailyOutput(ItemId product_id) const;
    [[nodiscard]] int GetReadyHarvestCount() const;
    [[nodiscard]] float GetQualityMultiplier(AnimalQuality quality) const;

    [[nodiscard]] const AnimalQualityRule& GetQualityRule(AnimalQuality quality) const;
    [[nodiscard]] AnimalQuality RollQuality(AnimalKind kind, int ranch_level, int random_seed) const;
    [[nodiscard]] int ApplyOutputBonus(int base_quantity, AnimalQuality quality) const;
    [[nodiscard]] int ApplyExpBonus(int base_exp, AnimalQuality quality) const;
    [[nodiscard]] ItemId GetPrimaryProduct(AnimalKind kind, AnimalQuality quality) const;
    [[nodiscard]] int GetOrderValueBonusPercent(AnimalQuality quality) const;

    [[nodiscard]] const RanchFacilityLevelRule& GetLevelRule(RanchFacilityKind kind, int level) const;
    [[nodiscard]] int GetProductionTicks(RanchFacilityKind kind, int level, int base_ticks) const;
    [[nodiscard]] bool CanSpawnQuality(RanchFacilityKind kind, int level, AnimalQuality quality) const;
    [[nodiscard]] bool IsBatchFeedUnlocked(int facility_id) const;
    [[nodiscard]] bool IsBatchHarvestUnlocked(int facility_id) const;
    [[nodiscard]] bool IsAutoHarvestUnlocked(int facility_id) const;

    void WriteStatus(std::ostream& os) const;

private:
    struct AnimalInstance {
        int animal_id = -1;
        AnimalKind kind = AnimalKind::Chicken;
        AnimalQuality quality = AnimalQuality::Common;
        AnimalState state = AnimalState::Idle;
        int finish_tick = 0;
        bool fed = false;
    };

    struct Facility {
        int facility_id = -1;
        RanchFacilityKind kind = RanchFacilityKind::ChickenCoop;
        int level = 1;
        int proficiency_exp = 0;
        std::vector<AnimalInstance> animals;
    };

    [[nodiscard]] Facility* FindFacility(int facility_id);
    [[nodiscard]] const Facility* FindFacility(int facility_id) const;
    [[nodiscard]] AnimalInstance* FindAnimal(Facility& facility, int animal_id);
    [[nodiscard]] const AnimalInstance* FindAnimal(const Facility& facility, int animal_id) const;
    [[nodiscard]] RanchFacilityView MakeFacilityView(const Facility& facility) const;

    [[nodiscard]] int BaseProductionTicks(AnimalKind kind) const;
    [[nodiscard]] ItemId FeedItemFor(AnimalKind kind) const;
    [[nodiscard]] int BuildCost(RanchFacilityKind kind) const;
    [[nodiscard]] int AnimalCost(AnimalKind kind, AnimalQuality quality) const;
    [[nodiscard]] bool FacilityMatchesAnimal(RanchFacilityKind facility_kind, AnimalKind animal_kind) const;

    ChickenCoop chicken_coop_;
    std::vector<Facility> facilities_;
    int next_facility_id_ = 1;
    int next_animal_id_ = 1;
    int last_tick_ = 0;
    int weather_scaled_tick_accumulator_ = 0;
};

}  // namespace farm
