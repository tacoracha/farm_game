#include "farm/RanchSystem.h"

#include "farm/Constants.h"
#include "farm/PlayerState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ostream>

namespace farm {

namespace {

constexpr int kMaxRanchLevel = 10;

constexpr std::array<AnimalQualityRule, 3> kQualityRules{{
    {AnimalQuality::Common, 70, 0, 0, 0},
    {AnimalQuality::Fine, 25, 20, 20, 20},
    {AnimalQuality::Rare, 5, 50, 50, 50},
}};

int ClampLevel(int level) {
    return std::max(1, std::min(kMaxRanchLevel, level));
}

}  // namespace

RanchSystem::RanchSystem() { Init(); }

void RanchSystem::Init() {
    facilities_.clear();
    next_facility_id_ = 1;
    next_animal_id_ = 1;
    last_tick_ = 0;
    weather_scaled_tick_accumulator_ = 0;

    Facility coop;
    coop.facility_id = next_facility_id_++;
    coop.kind = RanchFacilityKind::ChickenCoop;
    coop.level = 1;
    facilities_.push_back(coop);
}

void RanchSystem::Tick(int current_tick) {
    last_tick_ = current_tick;
    chicken_coop_.Tick(current_tick);

    for (Facility& facility : facilities_) {
        for (AnimalInstance& animal : facility.animals) {
            if (animal.state == AnimalState::Producing && current_tick >= animal.finish_tick) {
                animal.state = AnimalState::Ready;
                facility.proficiency_exp += ApplyExpBonus(1, animal.quality);
            }
        }
    }
}

void RanchSystem::OnTick(int tick_delta, float weather_multiplier) {
    if (tick_delta <= 0) {
        return;
    }
    if (weather_multiplier <= 0.0f) {
        weather_multiplier = 1.0f;
    }

    weather_scaled_tick_accumulator_ +=
        static_cast<int>(std::round(static_cast<float>(tick_delta) * weather_multiplier));
    Tick(weather_scaled_tick_accumulator_);
}

std::vector<RanchFacilityView> RanchSystem::GetAllFacilities() const {
    std::vector<RanchFacilityView> views;
    views.reserve(facilities_.size());
    for (const Facility& facility : facilities_) {
        views.push_back(MakeFacilityView(facility));
    }
    return views;
}

RanchFacilityView RanchSystem::GetFacilityView(int facility_id) const {
    const Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return {};
    }
    return MakeFacilityView(*facility);
}

std::vector<AnimalView> RanchSystem::GetAnimals(int facility_id) const {
    const Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return {};
    }

    std::vector<AnimalView> views;
    views.reserve(facility->animals.size());
    for (const AnimalInstance& animal : facility->animals) {
        AnimalView view;
        view.animal_id = animal.animal_id;
        view.kind = animal.kind;
        view.quality = animal.quality;
        view.state = animal.state;
        view.finish_tick = animal.finish_tick;
        view.remaining_ticks =
            animal.state == AnimalState::Producing ? std::max(0, animal.finish_tick - last_tick_) : 0;
        view.fed = animal.fed;
        view.product_id = GetPrimaryProduct(animal.kind, animal.quality);
        views.push_back(view);
    }
    return views;
}

Result<void> RanchSystem::BuildFacility(PlayerState& player, RanchFacilityKind kind) {
    auto spent = player.TrySpendGold(BuildCost(kind));
    if (!spent.ok()) {
        return spent;
    }

    Facility facility;
    facility.facility_id = next_facility_id_++;
    facility.kind = kind;
    facility.level = 1;
    facilities_.push_back(facility);
    return Result<void>::success();
}

Result<void> RanchSystem::UpgradeFacility(PlayerState& player, int facility_id) {
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr || facility->level >= kMaxRanchLevel) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }

    const RanchFacilityLevelRule& rule = GetLevelRule(facility->kind, facility->level);
    auto spent = player.TrySpendGold(rule.next_upgrade_cost.gold);
    if (!spent.ok()) {
        return spent;
    }

    ++facility->level;
    return Result<void>::success();
}

bool RanchSystem::IsRanchUnlocked(RanchFacilityKind kind, int player_level) const {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return player_level >= 1;
        case RanchFacilityKind::CowBarn:
            return player_level >= 2;
        case RanchFacilityKind::PigPen:
            return player_level >= 3;
        case RanchFacilityKind::SheepPen:
            return player_level >= 4;
    }
    return false;
}

Result<int> RanchSystem::BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind,
                                   AnimalQuality quality) {
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr || !FacilityMatchesAnimal(facility->kind, kind)) {
        return Result<int>::failure(ErrorCode::InvalidItem);
    }
    if (!CanSpawnQuality(facility->kind, facility->level, quality)) {
        return Result<int>::failure(ErrorCode::InvalidItem);
    }

    const int capacity = GetLevelRule(facility->kind, facility->level).animal_capacity;
    if (static_cast<int>(facility->animals.size()) >= capacity) {
        return Result<int>::failure(ErrorCode::WarehouseFull);
    }

    auto spent = player.TrySpendGold(AnimalCost(kind, quality));
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }

    AnimalInstance animal;
    animal.animal_id = next_animal_id_++;
    animal.kind = kind;
    animal.quality = quality;
    facility->animals.push_back(animal);
    return Result<int>::success(animal.animal_id);
}

Result<void> RanchSystem::FeedAnimal(PlayerState& player, int facility_id, int animal_id,
                                     int current_tick) {
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }
    AnimalInstance* animal = FindAnimal(*facility, animal_id);
    if (animal == nullptr) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }
    if (animal->state != AnimalState::Idle) {
        return Result<void>::failure(ErrorCode::ChickenNotIdleForFeed);
    }

    auto removed = player.TryRemoveFromWarehouse(FeedItemFor(animal->kind), 1);
    if (!removed.ok()) {
        return removed;
    }

    animal->state = AnimalState::Producing;
    animal->fed = true;
    animal->finish_tick =
        current_tick + GetProductionTicks(facility->kind, facility->level, BaseProductionTicks(animal->kind));
    return Result<void>::success();
}

Result<void> RanchSystem::HarvestAnimal(PlayerState& player, int facility_id, int animal_id) {
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }
    AnimalInstance* animal = FindAnimal(*facility, animal_id);
    if (animal == nullptr) {
        return Result<void>::failure(ErrorCode::ChickenSlotOutOfRange);
    }
    if (animal->state != AnimalState::Ready) {
        return Result<void>::failure(ErrorCode::ChickenNotReadyToCollectEgg);
    }

    auto added =
        player.TryAddToWarehouse(GetPrimaryProduct(animal->kind, animal->quality), ApplyOutputBonus(1, animal->quality));
    if (!added.ok()) {
        return added;
    }

    animal->state = AnimalState::Idle;
    animal->finish_tick = 0;
    animal->fed = false;
    facility->proficiency_exp += ApplyExpBonus(1, animal->quality);
    return Result<void>::success();
}

Result<int> RanchSystem::BatchFeed(PlayerState& player, int facility_id, int current_tick) {
    if (!IsBatchFeedUnlocked(facility_id)) {
        return Result<int>::failure(ErrorCode::NotImplemented);
    }
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::ChickenSlotOutOfRange);
    }

    int fed_count = 0;
    for (const AnimalInstance& animal : facility->animals) {
        if (animal.state != AnimalState::Idle) {
            continue;
        }
        auto fed = FeedAnimal(player, facility_id, animal.animal_id, current_tick);
        if (!fed.ok()) {
            break;
        }
        ++fed_count;
    }
    return Result<int>::success(fed_count);
}

Result<int> RanchSystem::BatchHarvest(PlayerState& player, int facility_id) {
    if (!IsBatchHarvestUnlocked(facility_id)) {
        return Result<int>::failure(ErrorCode::NotImplemented);
    }
    Facility* facility = FindFacility(facility_id);
    if (facility == nullptr) {
        return Result<int>::failure(ErrorCode::ChickenSlotOutOfRange);
    }

    int harvested_count = 0;
    for (const AnimalInstance& animal : facility->animals) {
        if (animal.state != AnimalState::Ready) {
            continue;
        }
        auto harvested = HarvestAnimal(player, facility_id, animal.animal_id);
        if (!harvested.ok()) {
            break;
        }
        ++harvested_count;
    }
    return Result<int>::success(harvested_count);
}

bool RanchSystem::IsAnimalProductUnlocked(ItemId product_id) const {
    if (product_id != ItemId::Egg) {
        return false;
    }
    return !facilities_.empty();
}

int RanchSystem::EstimateDailyOutput(ItemId product_id) const {
    if (!IsAnimalProductUnlocked(product_id)) {
        return 0;
    }

    int output = 0;
    for (const Facility& facility : facilities_) {
        for (const AnimalInstance& animal : facility.animals) {
            if (GetPrimaryProduct(animal.kind, animal.quality) != product_id) {
                continue;
            }
            const int ticks =
                GetProductionTicks(facility.kind, facility.level, BaseProductionTicks(animal.kind));
            output += ApplyOutputBonus(std::max(1, kTicksPerDay / std::max(1, ticks)), animal.quality);
        }
    }
    return output;
}

int RanchSystem::GetReadyHarvestCount() const {
    int count = 0;
    for (const Facility& facility : facilities_) {
        for (const AnimalInstance& animal : facility.animals) {
            if (animal.state == AnimalState::Ready) {
                ++count;
            }
        }
    }
    return count;
}

float RanchSystem::GetQualityMultiplier(AnimalQuality quality) const {
    return 1.0f + static_cast<float>(GetQualityRule(quality).output_bonus_percent) / 100.0f;
}

const AnimalQualityRule& RanchSystem::GetQualityRule(AnimalQuality quality) const {
    for (const AnimalQualityRule& rule : kQualityRules) {
        if (rule.quality == quality) {
            return rule;
        }
    }
    return kQualityRules.front();
}

AnimalQuality RanchSystem::RollQuality(AnimalKind, int ranch_level, int random_seed) const {
    AnimalQuality best_quality = AnimalQuality::Common;
    if (CanSpawnQuality(RanchFacilityKind::ChickenCoop, ranch_level, AnimalQuality::Rare)) {
        best_quality = AnimalQuality::Rare;
    } else if (CanSpawnQuality(RanchFacilityKind::ChickenCoop, ranch_level, AnimalQuality::Fine)) {
        best_quality = AnimalQuality::Fine;
    }

    const int roll = std::abs(random_seed) % 100;
    int cumulative = 0;
    for (const AnimalQualityRule& rule : kQualityRules) {
        if (static_cast<int>(rule.quality) > static_cast<int>(best_quality)) {
            continue;
        }
        cumulative += rule.spawn_weight;
        if (roll < cumulative) {
            return rule.quality;
        }
    }
    return AnimalQuality::Common;
}

int RanchSystem::ApplyOutputBonus(int base_quantity, AnimalQuality quality) const {
    if (base_quantity <= 0) {
        return 0;
    }
    return std::max(1, (base_quantity * (100 + GetQualityRule(quality).output_bonus_percent)) / 100);
}

int RanchSystem::ApplyExpBonus(int base_exp, AnimalQuality quality) const {
    if (base_exp <= 0) {
        return 0;
    }
    return std::max(1, (base_exp * (100 + GetQualityRule(quality).exp_bonus_percent)) / 100);
}

ItemId RanchSystem::GetPrimaryProduct(AnimalKind, AnimalQuality) const {
    return ItemId::Egg;
}

int RanchSystem::GetOrderValueBonusPercent(AnimalQuality quality) const {
    return GetQualityRule(quality).order_value_bonus_percent;
}

const RanchFacilityLevelRule& RanchSystem::GetLevelRule(RanchFacilityKind kind, int level) const {
    static RanchFacilityLevelRule rule;
    const int safe_level = ClampLevel(level);
    rule.kind = kind;
    rule.level = safe_level;
    rule.animal_capacity = (kind == RanchFacilityKind::ChickenCoop ? kChickenCoopSlotCount : 1) + safe_level - 1;
    rule.production_speed_bonus_percent = (safe_level - 1) * 5;
    rule.fine_quality_unlock_level = 2;
    rule.rare_quality_unlock_level = 4;
    rule.batch_feed_unlocked = safe_level >= 2;
    rule.batch_harvest_unlocked = safe_level >= 2;
    rule.auto_harvest_unlocked = safe_level >= 5;
    rule.next_upgrade_cost.gold = safe_level >= kMaxRanchLevel ? 0 : 30 + safe_level * 20;
    rule.next_upgrade_cost.tool_id = ItemId::Fertilizer;
    rule.next_upgrade_cost.tool_count = 0;
    return rule;
}

int RanchSystem::GetProductionTicks(RanchFacilityKind kind, int level, int base_ticks) const {
    const int bonus = GetLevelRule(kind, level).production_speed_bonus_percent;
    return std::max(1, (base_ticks * 100 + 99 + bonus) / (100 + bonus));
}

bool RanchSystem::CanSpawnQuality(RanchFacilityKind kind, int level, AnimalQuality quality) const {
    const RanchFacilityLevelRule& rule = GetLevelRule(kind, level);
    switch (quality) {
        case AnimalQuality::Common:
            return true;
        case AnimalQuality::Fine:
            return level >= rule.fine_quality_unlock_level;
        case AnimalQuality::Rare:
            return level >= rule.rare_quality_unlock_level;
    }
    return false;
}

bool RanchSystem::IsBatchFeedUnlocked(int facility_id) const {
    const Facility* facility = FindFacility(facility_id);
    return facility != nullptr && GetLevelRule(facility->kind, facility->level).batch_feed_unlocked;
}

bool RanchSystem::IsBatchHarvestUnlocked(int facility_id) const {
    const Facility* facility = FindFacility(facility_id);
    return facility != nullptr && GetLevelRule(facility->kind, facility->level).batch_harvest_unlocked;
}

bool RanchSystem::IsAutoHarvestUnlocked(int facility_id) const {
    const Facility* facility = FindFacility(facility_id);
    return facility != nullptr && GetLevelRule(facility->kind, facility->level).auto_harvest_unlocked;
}

RanchSystem::Facility* RanchSystem::FindFacility(int facility_id) {
    for (Facility& facility : facilities_) {
        if (facility.facility_id == facility_id) {
            return &facility;
        }
    }
    return nullptr;
}

const RanchSystem::Facility* RanchSystem::FindFacility(int facility_id) const {
    for (const Facility& facility : facilities_) {
        if (facility.facility_id == facility_id) {
            return &facility;
        }
    }
    return nullptr;
}

RanchSystem::AnimalInstance* RanchSystem::FindAnimal(Facility& facility, int animal_id) {
    for (AnimalInstance& animal : facility.animals) {
        if (animal.animal_id == animal_id) {
            return &animal;
        }
    }
    return nullptr;
}

const RanchSystem::AnimalInstance* RanchSystem::FindAnimal(const Facility& facility, int animal_id) const {
    for (const AnimalInstance& animal : facility.animals) {
        if (animal.animal_id == animal_id) {
            return &animal;
        }
    }
    return nullptr;
}

RanchFacilityView RanchSystem::MakeFacilityView(const Facility& facility) const {
    RanchFacilityView view;
    view.facility_id = facility.facility_id;
    view.kind = facility.kind;
    view.level = facility.level;
    view.capacity = GetLevelRule(facility.kind, facility.level).animal_capacity;
    view.proficiency_exp = facility.proficiency_exp;
    view.idle_count = 0;
    view.producing_count = 0;
    view.harvestable_count = 0;
    for (const AnimalInstance& animal : facility.animals) {
        switch (animal.state) {
            case AnimalState::Idle:
                ++view.idle_count;
                break;
            case AnimalState::Producing:
                ++view.producing_count;
                break;
            case AnimalState::Ready:
                ++view.harvestable_count;
                break;
        }
    }
    return view;
}

int RanchSystem::BaseProductionTicks(AnimalKind kind) const {
    switch (kind) {
        case AnimalKind::Chicken:
            return kChickenEggProductionTicks;
        case AnimalKind::Cow:
            return 4;
        case AnimalKind::Pig:
            return 5;
        case AnimalKind::Sheep:
            return 6;
    }
    return kChickenEggProductionTicks;
}

ItemId RanchSystem::FeedItemFor(AnimalKind kind) const {
    return kind == AnimalKind::Chicken ? ItemId::ChickenFeed : ItemId::CowFeed;
}

int RanchSystem::BuildCost(RanchFacilityKind kind) const {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return 30;
        case RanchFacilityKind::CowBarn:
            return 60;
        case RanchFacilityKind::PigPen:
            return 70;
        case RanchFacilityKind::SheepPen:
            return 80;
    }
    return 30;
}

int RanchSystem::AnimalCost(AnimalKind kind, AnimalQuality quality) const {
    int base = 10;
    switch (kind) {
        case AnimalKind::Chicken:
            base = 10;
            break;
        case AnimalKind::Cow:
            base = 25;
            break;
        case AnimalKind::Pig:
            base = 20;
            break;
        case AnimalKind::Sheep:
            base = 20;
            break;
    }
    return ApplyOutputBonus(base, quality);
}

bool RanchSystem::FacilityMatchesAnimal(RanchFacilityKind facility_kind, AnimalKind animal_kind) const {
    return (facility_kind == RanchFacilityKind::ChickenCoop && animal_kind == AnimalKind::Chicken) ||
           (facility_kind == RanchFacilityKind::CowBarn && animal_kind == AnimalKind::Cow) ||
           (facility_kind == RanchFacilityKind::PigPen && animal_kind == AnimalKind::Pig) ||
           (facility_kind == RanchFacilityKind::SheepPen && animal_kind == AnimalKind::Sheep);
}

void RanchSystem::WriteStatus(std::ostream& os) const {
    chicken_coop_.WriteStatus(os);
    os << "Ranch facilities:\n";
    for (const RanchFacilityView& view : GetAllFacilities()) {
        os << "  facility " << view.facility_id << " level " << view.level << " animals "
           << (view.idle_count + view.producing_count + view.harvestable_count) << "/" << view.capacity
           << " ready " << view.harvestable_count << "\n";
    }
}

}  // namespace farm
