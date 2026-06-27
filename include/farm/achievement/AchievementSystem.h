#pragma once

#include "farm/common/Types.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace farm {

enum class AchievementCategory : std::uint8_t {
    Plant,    // 种植类
    Ranch,    // 牧场类
    Process,  // 加工类
    Order,    // 订单类
    Explore,  // 探索/等级/解锁/季节类
    General,  // 综合终极类
};

enum class AchievementID : std::uint8_t {
    // Plant
    FirstHarvest,        // 收获第1个作物
    WheatLover,          // 累计收获10个小麦
    HardworkingFarmer,   // 累计收获100个任意作物
    HighYieldFarmer,     // 累计收获500个任意作物
    CropCollector,       // 收获过全部种类作物
    SeasonHarvest,       // 单季节内收获50个作物

    // Ranch
    FirstProduct,        // 收获第一个动物产品
    ChickenMaster,       // 同时拥有5只鸡
    SmallRanch,          // 同时拥有10只动物
    LargeRanch,          // 同时拥有20只动物
    AllAnimals,          // 拥有过全部种类动物

    // Process
    FirstProcess,        // 完成第一次加工
    SkilledWorker,       // 累计加工100件
    ProcessMaster,       // 累计加工500件
    AllRecipes,          // 解锁全部加工配方

    // Order
    FirstOrder,          // 完成第一个订单
    Shopkeeper,          // 累计完成10个订单
    OrderPro,            // 累计完成100个订单
    DailyOrders,         // 单个游戏日内完成5个订单

    // Explore
    FirstUnlock,         // 解锁任意新内容
    AllSeeds,            // 解锁全部作物种子
    AllBuildings,        // 解锁全部牧场/扩建建筑
    Level10,             // 玩家等级到达10级
    Level20,             // 玩家等级到达20级
    FourSeasons,         // 完整经历春、夏、秋、冬四个季节

    // General
    Wealthy,             // 金币累计达到10000
    FullWarehouse,       // 仓库达到最大容量

    COUNT
};

struct AchievementDef {
    AchievementID id;
    AchievementCategory category;
    const char* name;
    const char* desc;
    int target;
    int reward_gold;
    int reward_exp;
    const char* reward_text;  // extra reward description, empty if none
};

struct Achievement {
    AchievementID id = AchievementID::FirstHarvest;
    AchievementCategory category = AchievementCategory::Plant;
    std::string name;
    std::string desc;
    int target = 1;
    int current = 0;
    bool completed = false;
    int reward_gold = 0;
    int reward_exp = 0;
    std::string reward_text;
};

class PlayerState;

class AchievementSystem {
public:
    AchievementSystem();

    // Init all achievement configs
    void Init();

    // Event hooks (called by other systems)
    void OnHarvestCrop(ItemId crop);
    void OnHarvestProduct(ItemId product);
    void OnBuyAnimal(AnimalKind kind);
    void OnProcessItem();
    void OnCompleteOrder();
    void OnPlayerLevelUp(int level);
    void OnUnlockContent();
    void OnSeasonChange(Season season);
    void OnAddGold(int gold);

    // Tick-based checks (called from Game::AdvanceTicks)
    void CheckPeriodic(int current_tick, int total_animals, int chicken_count,
                       int warehouse_capacity, int warehouse_used);

    // UI queries
    const std::vector<Achievement>& All() const { return achievements_; }
    bool HasNew() const { return has_new_; }
    void ClearNewFlag() { has_new_ = false; }

    // Save / Load (called by SaveManager via Game)
    void ClearForLoad();
    void SetProgressForLoad(AchievementID id, int current, bool completed);
    void SetProgressIfNotDone(AchievementID id, int current);
    const std::vector<Achievement>& AchievementsForSave() const { return achievements_; }

    // Popup callback registration
    void SetPopupCallback(void (*fn)(const char* name, int gold, int exp, const char* extra));

    // External setters for tracking (called by save/load and periodic checks)
    void SetSeedsUnlocked(int n);
    void SetBuildingsUnlocked(int n);
    void SetRecipesKnown(int n);
    void SetTotalHarvests(int n);
    void SetWheatHarvests(int n);
    void SetTotalProcessed(int n);
    void SetTotalOrders(int n);
    void SetTotalGoldEarned(int n);
    void SetUnlockCount(int n);
    void SetSeasonHarvestCount(int n);
    void SetSeenSeason(int s_val);
    void SetHarvestedCrop(int crop_val);
    void SetOwnedAnimal(int kind_val);

    // Getters for save
    int TotalHarvests() const;
    int WheatHarvests() const;
    int TotalProcessed() const;
    int TotalOrders() const;
    int TotalGoldEarned() const;
    int UnlockCount() const;
    int SeasonHarvestCount() const;
    int SeedsUnlocked() const;
    int BuildingsUnlocked() const;
    int RecipesKnown() const;
    const std::set<Season>& SeenSeasons() const;
    const std::set<ItemId>& HarvestedCropsForSave() const;
    const std::set<AnimalKind>& OwnedAnimalsForSave() const;

private:
    void CheckAndAward(Achievement& a);
    const AchievementDef* FindDef(AchievementID id) const;

    std::vector<Achievement> achievements_;
    std::set<ItemId> harvested_crops_;       // for CropCollector
    std::set<AnimalKind> owned_animals_;     // for AllAnimals
    std::set<RecipeId> unlocked_recipes_;    // for AllRecipes
    std::set<Season> seen_seasons_;          // for FourSeasons

    int season_harvest_count_ = 0;           // for SeasonHarvest (reset on season change)
    int daily_order_count_ = 0;              // for DailyOrders (reset on new day)
    int current_day_ = 1;

    int total_processed_ = 0;
    int total_orders_ = 0;
    int total_harvests_ = 0;
    int wheat_harvests_ = 0;
    int total_gold_earned_ = 0;
    int unlock_count_ = 0;
    int process_recipes_known_ = 0;
    int seeds_unlocked_ = 0;
    int buildings_unlocked_ = 0;
    bool initial_unlock_skipped_ = false;

    bool has_new_ = false;
    bool initialized_ = false;

    void (*popup_fn_)(const char*, int, int, const char*) = nullptr;
};

const char* CategoryName(AchievementCategory cat);

}  // namespace farm
