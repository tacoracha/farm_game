#include "farm/dailytask/DailyTaskSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cstdlib>
#include <set>

namespace farm {

DailyTaskSystem& DailyTaskSystem::Instance() {
    static DailyTaskSystem instance;
    return instance;
}

void DailyTaskSystem::Init() {
    task_pool_ = {
        {DailyTaskID::Harvest5Crop, DailyTaskType::HarvestCrop, "勤劳收获", "收获5个任意作物", 5, 0, false, 20, 10, ""},
        {DailyTaskID::Harvest10Crop, DailyTaskType::HarvestCrop, "丰收时刻", "收获10个任意作物", 10, 0, false, 40, 20, "肥料x2"},
        {DailyTaskID::Water5Crop, DailyTaskType::WaterCrop, "细心浇灌", "给5块地浇水", 5, 0, false, 15, 8, ""},
        {DailyTaskID::Water10Crop, DailyTaskType::WaterCrop, "灌溉专家", "给10块地浇水", 10, 0, false, 30, 15, ""},
        {DailyTaskID::Fertilize3Crop, DailyTaskType::FertilizeCrop, "施肥达人", "给3块地施肥", 3, 0, false, 25, 12, ""},
        {DailyTaskID::Feed3Animal, DailyTaskType::FeedAnimal, "动物管家", "喂食3次动物", 3, 0, false, 20, 10, ""},
        {DailyTaskID::Feed5Animal, DailyTaskType::FeedAnimal, "牧场主人", "喂食5次动物", 5, 0, false, 35, 18, ""},
        {DailyTaskID::Harvest3Product, DailyTaskType::HarvestProduct, "新鲜产出", "收获3个动物产品", 3, 0, false, 25, 12, ""},
        {DailyTaskID::Process5Feed, DailyTaskType::ProcessFeed, "饲料加工", "加工5份饲料", 5, 0, false, 20, 10, ""},
        {DailyTaskID::Process10Feed, DailyTaskType::ProcessFeed, "批量生产", "加工10份饲料", 10, 0, false, 40, 20, "鸡饲料x5"},
        {DailyTaskID::Complete2Order, DailyTaskType::CompleteOrder, "小本生意", "完成2个订单", 2, 0, false, 30, 15, ""},
        {DailyTaskID::Complete3Order, DailyTaskType::CompleteOrder, "生意兴隆", "完成3个订单", 3, 0, false, 50, 25, "肥料x1"},
        {DailyTaskID::Buy1Animal, DailyTaskType::BuyAnimal, "新成员", "购买1只动物", 1, 0, false, 35, 18, ""},
    };
}

void DailyTaskSystem::RefreshTasks(PlayerState& player) {
    // Pick 3 random tasks of different types from the pool
    today_tasks_.clear();
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(task_pool_.size()); ++i) indices.push_back(i);

    // Fisher-Yates shuffle using a simple PRNG seeded by day
    int day_seed = last_refresh_day_ * 31 + 17;
    for (int i = static_cast<int>(indices.size()) - 1; i > 0; --i) {
        int j = (day_seed * (i + 1) + i * 7) % (i + 1);
        std::swap(indices[i], indices[j]);
    }

    // Pick up to 3 tasks with different types
    int picked = 0;
    std::set<DailyTaskType> used_types;
    for (int idx : indices) {
        DailyTask& t = task_pool_[static_cast<std::size_t>(idx)];
        if (used_types.count(t.type)) continue;
        t.current = 0;
        t.completed = false;
        today_tasks_.push_back(t);
        used_types.insert(t.type);
        ++picked;
        if (picked >= 3) break;
    }
    has_new_ = false;
}

void DailyTaskSystem::CheckComplete(DailyTask& task, PlayerState& player) {
    if (task.completed) return;
    if (task.current >= task.target) {
        task.completed = true;
        has_new_ = true;
        player.AddGold(task.reward_gold);
        player.AddExperience(task.reward_exp);
        if (!task.reward_text.empty()) {
            // Parse reward text: e.g. "肥料x2" -> try to add Fertilizer x2
            if (task.reward_text.find("肥料") != std::string::npos) {
                int qty = 1;
                auto pos = task.reward_text.find('x');
                if (pos != std::string::npos) qty = std::stoi(task.reward_text.substr(pos + 1));
                player.TryAddItem(ItemId::Fertilizer, qty);
            } else if (task.reward_text.find("鸡饲料") != std::string::npos) {
                int qty = 1;
                auto pos = task.reward_text.find('x');
                if (pos != std::string::npos) qty = std::stoi(task.reward_text.substr(pos + 1));
                player.TryAddItem(ItemId::ChickenFeed, qty);
            }
        }
    }
}

void DailyTaskSystem::Tick(int current_tick, PlayerState& player) {
    int day = current_tick / kTicksPerDay + 1;
    if (day != last_refresh_day_) {
        last_refresh_day_ = day;
        RefreshTasks(player);
    }
    // Check completions
    for (DailyTask& t : today_tasks_) {
        CheckComplete(t, player);
    }
}

// --- Event hooks ---

void DailyTaskSystem::OnHarvestCrop(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::HarvestCrop && !t.completed) {
            t.current = std::min(t.target, t.current + count);
            // CheckComplete needs PlayerState; handled in Tick or via stored pointer
        }
    }
}

void DailyTaskSystem::OnWaterCrop(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::WaterCrop && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnFertilizeCrop(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::FertilizeCrop && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnFeedAnimal(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::FeedAnimal && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnHarvestProduct(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::HarvestProduct && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnProcessFeed(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::ProcessFeed && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnCompleteOrder(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::CompleteOrder && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

void DailyTaskSystem::OnBuyAnimal(int count) {
    for (DailyTask& t : today_tasks_) {
        if (t.type == DailyTaskType::BuyAnimal && !t.completed) {
            t.current = std::min(t.target, t.current + count);
        }
    }
}

bool DailyTaskSystem::HasUncompleted() const {
    for (const DailyTask& t : today_tasks_) {
        if (!t.completed) return true;
    }
    return false;
}

// --- Save / Load ---

void DailyTaskSystem::ClearForLoad() {
    today_tasks_.clear();
    last_refresh_day_ = 0;
    has_new_ = false;
}

void DailyTaskSystem::SetForLoad(int last_day, const std::vector<DailyTask>& tasks) {
    last_refresh_day_ = last_day;
    today_tasks_ = tasks;
    has_new_ = false;
    // Re-check completed tasks for rewards already given
}

}  // namespace farm
