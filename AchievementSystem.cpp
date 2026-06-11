#include "farm/achievement/AchievementSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>

namespace farm {

namespace {

constexpr AchievementDef kAchievementDefs[] = {
    // Plant
    {AchievementID::FirstHarvest, AchievementCategory::Plant, "新手农夫", "收获第1个作物", 1, 10, 5, ""},
    {AchievementID::WheatLover, AchievementCategory::Plant, "小麦爱好者", "累计收获10个小麦", 10, 20, 10, "小麦种子x5"},
    {AchievementID::HardworkingFarmer, AchievementCategory::Plant, "勤劳农夫", "累计收获100个任意作物", 100, 50, 25, "肥料x3"},
    {AchievementID::HighYieldFarmer, AchievementCategory::Plant, "高产农户", "累计收获500个任意作物", 500, 100, 50, ""},
    {AchievementID::CropCollector, AchievementCategory::Plant, "作物收藏家", "收获过全部种类作物", 8, 150, 75, ""},
    {AchievementID::SeasonHarvest, AchievementCategory::Plant, "季节丰收", "单个季节内收获50个作物", 50, 80, 40, ""},

    // Ranch
    {AchievementID::FirstProduct, AchievementCategory::Ranch, "第一枚产出", "收获第一个动物产品", 1, 15, 7, ""},
    {AchievementID::ChickenMaster, AchievementCategory::Ranch, "养鸡能手", "同时拥有5只鸡", 5, 30, 15, ""},
    {AchievementID::SmallRanch, AchievementCategory::Ranch, "小型牧场", "同时拥有10只任意动物", 10, 60, 30, ""},
    {AchievementID::LargeRanch, AchievementCategory::Ranch, "大型牧场", "同时拥有20只任意动物", 20, 120, 60, ""},
    {AchievementID::AllAnimals, AchievementCategory::Ranch, "畜牧全解锁", "拥有过全部种类动物", 3, 100, 50, ""},

    // Process
    {AchievementID::FirstProcess, AchievementCategory::Process, "初次加工", "完成第一次物品加工", 1, 10, 5, ""},
    {AchievementID::SkilledWorker, AchievementCategory::Process, "熟练工人", "累计加工100件物品", 100, 40, 20, ""},
    {AchievementID::ProcessMaster, AchievementCategory::Process, "加工达人", "累计加工500件物品", 500, 80, 40, ""},
    {AchievementID::AllRecipes, AchievementCategory::Process, "配方精通", "解锁全部加工配方", 5, 100, 50, ""},

    // Order
    {AchievementID::FirstOrder, AchievementCategory::Order, "第一笔生意", "完成第一个订单", 1, 20, 10, ""},
    {AchievementID::Shopkeeper, AchievementCategory::Order, "小店主", "累计完成10个订单", 10, 50, 25, ""},
    {AchievementID::OrderPro, AchievementCategory::Order, "接单达人", "累计完成100个订单", 100, 100, 50, ""},
    {AchievementID::DailyOrders, AchievementCategory::Order, "单日爆单", "单个游戏日内完成5个订单", 5, 70, 35, ""},

    // Explore
    {AchievementID::FirstUnlock, AchievementCategory::Explore, "初次解锁", "解锁任意新内容", 1, 15, 7, ""},
    {AchievementID::AllSeeds, AchievementCategory::Explore, "种子全解锁", "解锁全部作物种子", 7, 80, 40, ""},
    {AchievementID::AllBuildings, AchievementCategory::Explore, "建筑全解锁", "解锁全部牧场/扩建建筑", 4, 150, 75, ""},
    {AchievementID::Level10, AchievementCategory::Explore, "小有成就", "玩家等级到达10级", 10, 100, 50, ""},
    {AchievementID::Level20, AchievementCategory::Explore, "资深农场主", "玩家等级到达20级", 20, 200, 100, ""},
    {AchievementID::FourSeasons, AchievementCategory::Explore, "四季轮回", "完整经历春、夏、秋、冬四个季节", 4, 120, 60, ""},

    // General
    {AchievementID::Wealthy, AchievementCategory::General, "财富积累", "金币累计达到10000", 10000, 300, 150, ""},
    {AchievementID::FullWarehouse, AchievementCategory::General, "仓库满载", "仓库达到最大容量", 150, 100, 50, ""},
};

constexpr int kAchievementCount = static_cast<int>(AchievementID::COUNT);

}  // namespace

const char* farm::CategoryName(AchievementCategory cat) {
    switch (cat) {
        case AchievementCategory::Plant:    return "种植";
        case AchievementCategory::Ranch:    return "牧场";
        case AchievementCategory::Process:  return "加工";
        case AchievementCategory::Order:    return "订单";
        case AchievementCategory::Explore:  return "探索";
        case AchievementCategory::General:  return "综合";
    }
    return "";
}

AchievementSystem& AchievementSystem::Instance() {
    static AchievementSystem instance;
    return instance;
}

const AchievementDef* AchievementSystem::FindDef(AchievementID id) const {
    for (const AchievementDef& def : kAchievementDefs) {
        if (def.id == id) return &def;
    }
    return nullptr;
}

void AchievementSystem::Init() {
    if (initialized_) return;
    initialized_ = true;

    achievements_.clear();
    for (const AchievementDef& def : kAchievementDefs) {
        Achievement a;
        a.id = def.id;
        a.category = def.category;
        a.name = def.name;
        a.desc = def.desc;
        a.target = def.target;
        a.current = 0;
        a.completed = false;
        a.reward_gold = def.reward_gold;
        a.reward_exp = def.reward_exp;
        a.reward_text = def.reward_text;
        achievements_.push_back(a);
    }
}

void AchievementSystem::CheckAndAward(Achievement& a) {
    if (a.completed) return;
    if (a.current >= a.target) {
        a.completed = true;
        has_new_ = true;
        if (popup_fn_) {
            popup_fn_(a.name.c_str(), a.reward_gold, a.reward_exp, a.reward_text.c_str());
        }
    }
}

// --- Event hooks ---

void AchievementSystem::OnHarvestCrop(ItemId crop) {
    if (!initialized_) return;
    ++total_harvests_;
    // Individual achievements
    for (Achievement& a : achievements_) {
        switch (a.id) {
            case AchievementID::FirstHarvest:
            case AchievementID::HardworkingFarmer:
            case AchievementID::HighYieldFarmer:
                a.current = total_harvests_;
                break;
            case AchievementID::WheatLover:
                if (crop == ItemId::Wheat) { ++wheat_harvests_; a.current = wheat_harvests_; }
                break;
            case AchievementID::CropCollector:
                harvested_crops_.insert(crop);
                a.current = static_cast<int>(harvested_crops_.size());
                break;
            case AchievementID::SeasonHarvest:
                ++season_harvest_count_;
                a.current = season_harvest_count_;
                break;
            default: break;
        }
        CheckAndAward(a);
    }
}

void AchievementSystem::OnHarvestProduct(ItemId /*product*/) {
    if (!initialized_) return;
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::FirstProduct) {
            a.current = 1;
            CheckAndAward(a);
        }
    }
}

void AchievementSystem::OnBuyAnimal(AnimalKind kind) {
    if (!initialized_) return;
    owned_animals_.insert(kind);
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::AllAnimals) {
            a.current = static_cast<int>(owned_animals_.size());
            CheckAndAward(a);
        }
    }
}

void AchievementSystem::OnProcessItem() {
    if (!initialized_) return;
    ++total_processed_;
    for (Achievement& a : achievements_) {
        switch (a.id) {
            case AchievementID::FirstProcess:
            case AchievementID::SkilledWorker:
            case AchievementID::ProcessMaster:
                a.current = total_processed_;
                break;
            default: break;
        }
        CheckAndAward(a);
    }
}

void AchievementSystem::OnCompleteOrder() {
    if (!initialized_) return;
    ++total_orders_;
    ++daily_order_count_;
    for (Achievement& a : achievements_) {
        switch (a.id) {
            case AchievementID::FirstOrder:
            case AchievementID::Shopkeeper:
            case AchievementID::OrderPro:
                a.current = total_orders_;
                break;
            case AchievementID::DailyOrders:
                a.current = daily_order_count_;
                break;
            default: break;
        }
        CheckAndAward(a);
    }
}

void AchievementSystem::OnPlayerLevelUp(int level) {
    if (!initialized_) return;
    for (Achievement& a : achievements_) {
        switch (a.id) {
            case AchievementID::Level10:
                a.current = level;
                break;
            case AchievementID::Level20:
                a.current = level;
                break;
            default: break;
        }
        CheckAndAward(a);
    }
}

void AchievementSystem::OnUnlockContent() {
    if (!initialized_) return;
    if (!initial_unlock_skipped_) {
        initial_unlock_skipped_ = true;
        return;  // Skip the initial auto-unlocks at game start
    }
    ++unlock_count_;
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::FirstUnlock) {
            a.current = unlock_count_;
            CheckAndAward(a);
        }
    }
}

void AchievementSystem::OnSeasonChange(Season season) {
    if (!initialized_) return;
    season_harvest_count_ = 0;  // Reset season harvest counter
    seen_seasons_.insert(season);
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::FourSeasons) {
            a.current = static_cast<int>(seen_seasons_.size());
            CheckAndAward(a);
        }
    }
}

void AchievementSystem::OnAddGold(int gold) {
    if (!initialized_ || gold <= 0) return;
    total_gold_earned_ += gold;
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::Wealthy) {
            a.current = total_gold_earned_;
            CheckAndAward(a);
        }
    }
}

void AchievementSystem::CheckPeriodic(int current_tick, int total_animals,
                                       int chicken_count, int warehouse_capacity,
                                       int warehouse_used) {
    if (!initialized_) return;
    int day = current_tick / kTicksPerDay + 1;
    if (day != current_day_) {
        daily_order_count_ = 0;
        current_day_ = day;
    }

    // Animal count achievements
    for (Achievement& a : achievements_) {
        switch (a.id) {
            case AchievementID::ChickenMaster:
                a.current = chicken_count;
                break;
            case AchievementID::SmallRanch:
                a.current = total_animals;
                break;
            case AchievementID::LargeRanch:
                a.current = total_animals;
                break;
            case AchievementID::AllSeeds:
                // Updated externally - but we track via the def target
                break;
            case AchievementID::AllBuildings:
                break;
            case AchievementID::AllRecipes:
                break;
            case AchievementID::FullWarehouse:
                a.current = (warehouse_used >= warehouse_capacity) ? warehouse_capacity : 0;
                break;
            default: break;
        }
        CheckAndAward(a);
    }

    // Seed/buildings/recipe achievements are updated externally
    for (Achievement& a : achievements_) {
        if (a.id == AchievementID::AllSeeds) {
            a.current = seeds_unlocked_;
        } else if (a.id == AchievementID::AllBuildings) {
            a.current = buildings_unlocked_;
        } else if (a.id == AchievementID::AllRecipes) {
            a.current = process_recipes_known_;
        }
        CheckAndAward(a);
    }
}

void AchievementSystem::SetPopupCallback(void (*fn)(const char*, int, int, const char*)) {
    popup_fn_ = fn;
}

// --- Save / Load ---

void AchievementSystem::ClearForLoad() {
    Init();
    total_harvests_ = 0;
    wheat_harvests_ = 0;
    total_processed_ = 0;
    total_orders_ = 0;
    total_gold_earned_ = 0;
    unlock_count_ = 0;
    season_harvest_count_ = 0;
    daily_order_count_ = 0;
    harvested_crops_.clear();
    owned_animals_.clear();
    unlocked_recipes_.clear();
    seen_seasons_.clear();
    has_new_ = false;
}

void AchievementSystem::SetProgressForLoad(AchievementID id, int current, bool completed) {
    for (Achievement& a : achievements_) {
        if (a.id == id) {
            a.current = current;
            a.completed = completed;
            return;
        }
    }
}

void AchievementSystem::SetProgressIfNotDone(AchievementID id, int current) {
    for (Achievement& a : achievements_) {
        if (a.id == id) {
            if (!a.completed) a.current = current;
            return;
        }
    }
}

// Expose setters for external tracking
void AchievementSystem::SetSeedsUnlocked(int n) { seeds_unlocked_ = n; }
void AchievementSystem::SetBuildingsUnlocked(int n) { buildings_unlocked_ = n; }
void AchievementSystem::SetRecipesKnown(int n) { process_recipes_known_ = n; }
void AchievementSystem::SetTotalHarvests(int n) { total_harvests_ = n; }
void AchievementSystem::SetWheatHarvests(int n) { wheat_harvests_ = n; }
void AchievementSystem::SetTotalProcessed(int n) { total_processed_ = n; }
void AchievementSystem::SetTotalOrders(int n) { total_orders_ = n; }
void AchievementSystem::SetTotalGoldEarned(int n) { total_gold_earned_ = n; }
void AchievementSystem::SetUnlockCount(int n) { unlock_count_ = n; }
void AchievementSystem::SetSeasonHarvestCount(int n) { season_harvest_count_ = n; }
void AchievementSystem::SetSeenSeason(int s_val) { seen_seasons_.insert(static_cast<Season>(s_val)); }
void AchievementSystem::SetHarvestedCrop(int crop_val) { harvested_crops_.insert(static_cast<ItemId>(crop_val)); }
void AchievementSystem::SetOwnedAnimal(int kind_val) { owned_animals_.insert(static_cast<AnimalKind>(kind_val)); }

int AchievementSystem::TotalHarvests() const { return total_harvests_; }
int AchievementSystem::WheatHarvests() const { return wheat_harvests_; }
int AchievementSystem::TotalProcessed() const { return total_processed_; }
int AchievementSystem::TotalOrders() const { return total_orders_; }
int AchievementSystem::TotalGoldEarned() const { return total_gold_earned_; }
int AchievementSystem::UnlockCount() const { return unlock_count_; }
int AchievementSystem::SeasonHarvestCount() const { return season_harvest_count_; }
int AchievementSystem::SeedsUnlocked() const { return seeds_unlocked_; }
int AchievementSystem::BuildingsUnlocked() const { return buildings_unlocked_; }
int AchievementSystem::RecipesKnown() const { return process_recipes_known_; }
const std::set<Season>& AchievementSystem::SeenSeasons() const { return seen_seasons_; }
const std::set<ItemId>& AchievementSystem::HarvestedCropsForSave() const { return harvested_crops_; }
const std::set<AnimalKind>& AchievementSystem::OwnedAnimalsForSave() const { return owned_animals_; }

}  // namespace farm
