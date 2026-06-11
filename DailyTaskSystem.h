#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace farm {

enum class DailyTaskType : std::uint8_t {
    HarvestCrop, WaterCrop, FertilizeCrop, FeedAnimal,
    HarvestProduct, ProcessFeed, CompleteOrder, BuyAnimal
};

enum class DailyTaskID : std::uint8_t {
    Harvest5Crop, Harvest10Crop, Water5Crop, Water10Crop,
    Fertilize3Crop, Feed3Animal, Feed5Animal, Harvest3Product,
    Process5Feed, Process10Feed, Complete2Order, Complete3Order, Buy1Animal,
    COUNT
};

struct DailyTask {
    DailyTaskID id = DailyTaskID::Harvest5Crop;
    DailyTaskType type = DailyTaskType::HarvestCrop;
    std::string name;
    std::string description;
    int target = 1;
    int current = 0;
    bool completed = false;
    int reward_gold = 0;
    int reward_exp = 0;
    std::string reward_text;
};

class PlayerState;

class DailyTaskSystem {
public:
    static DailyTaskSystem& Instance();

    void Init();
    void Tick(int current_tick, PlayerState& player);

    // Event hooks
    void OnHarvestCrop(int count);
    void OnWaterCrop(int count);
    void OnFertilizeCrop(int count);
    void OnFeedAnimal(int count);
    void OnHarvestProduct(int count);
    void OnProcessFeed(int count);
    void OnCompleteOrder(int count);
    void OnBuyAnimal(int count);

    // UI queries
    const std::vector<DailyTask>& TodayTasks() const { return today_tasks_; }
    bool HasUncompleted() const;
    bool HasNewComplete() const { return has_new_; }
    void ClearNewFlag() { has_new_ = false; }

    // Save / Load
    void ClearForLoad();
    void SetForLoad(int last_day, const std::vector<DailyTask>& tasks);

    int LastRefreshDay() const { return last_refresh_day_; }
    int CurrentDayForSave() const { return last_refresh_day_; }

private:
    DailyTaskSystem() = default;
    void RefreshTasks(PlayerState& player);
    void CheckComplete(DailyTask& task, PlayerState& player);

    std::vector<DailyTask> task_pool_;
    std::vector<DailyTask> today_tasks_;
    int last_refresh_day_ = 0;
    bool has_new_ = false;
};

}  // namespace farm
