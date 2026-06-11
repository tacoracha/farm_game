#include "farm/fishing/FishingSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cstdlib>

namespace farm {

namespace {

const FishDef kFishDefs[] = {
    // Spring common
    {1, "鲫鱼", FishRarity::Common, 8, static_cast<Season>(1), false, 30},
    {2, "鲤鱼", FishRarity::Common, 10, static_cast<Season>(1), false, 35},
    {3, "草鱼", FishRarity::Common, 12, static_cast<Season>(0xF), false, 40},
    // Summer
    {4, "鲈鱼", FishRarity::Common, 15, static_cast<Season>(2), false, 38},
    {5, "鳊鱼", FishRarity::Common, 13, static_cast<Season>(2), false, 42},
    // Autumn
    {6, "青鱼", FishRarity::Common, 14, static_cast<Season>(4), false, 36},
    {7, "鳜鱼", FishRarity::Rare, 28, static_cast<Season>(4), false, 55},
    // Winter
    {8, "鲢鱼", FishRarity::Common, 11, static_cast<Season>(8), false, 40},
    {9, "鳕鱼", FishRarity::Common, 16, static_cast<Season>(8), false, 45},
    // Rare (multi-season)
    {10, "鲶鱼", FishRarity::Rare, 35, static_cast<Season>(0xF), true, 60},
    {11, "虹鳟", FishRarity::Rare, 40, static_cast<Season>(3), false, 58},
    {12, "黑鱼", FishRarity::Rare, 45, static_cast<Season>(6), false, 62},
    // Legendary
    {13, "金龙鱼", FishRarity::Legendary, 120, static_cast<Season>(3), false, 85},
    {14, "锦鲤王", FishRarity::Legendary, 150, static_cast<Season>(0xF), true, 90},
    {15, "鲟鱼", FishRarity::Legendary, 200, static_cast<Season>(12), false, 92},
};

constexpr int kFishCount = sizeof(kFishDefs) / sizeof(kFishDefs[0]);

}  // namespace

FishingSystem& FishingSystem::Instance() {
    static FishingSystem instance;
    return instance;
}

const std::vector<FishDef>& FishingSystem::FishList() {
    static std::vector<FishDef> list(kFishDefs, kFishDefs + kFishCount);
    return list;
}

const FishDef* FishingSystem::GetFishDef(int id) const {
    for (const FishDef& f : kFishDefs) {
        if (f.id == id) return &f;
    }
    return nullptr;
}

void FishingSystem::Init() {
    bait_count_ = 3;  // start with 3 bait
    rod_level_ = 1;
    total_catches_ = 0;
    last_fish_id_ = -1;
    state_ = FishingState::Idle;
    collection_.clear();
}

bool FishingSystem::CanFish() const {
    return bait_count_ > 0 && state_ == FishingState::Idle;
}

void FishingSystem::CastLine(int current_tick) {
    if (!CanFish()) return;
    UseBait();
    state_ = FishingState::Waiting;
    cast_tick_ = current_tick;
    bite_tick_ = current_tick + 2 + (std::abs(current_tick * 17 + 3) % 7);  // 2-8 ticks
    target_fish_id_ = RollFish(current_tick);
}

const FishDef* FishingSystem::GetPendingFish() const {
    return GetFishDef(target_fish_id_);
}

bool FishingSystem::UseBait() {
    if (bait_count_ <= 0) return false;
    --bait_count_;
    return true;
}

int FishingSystem::GetBiteCountdown() const {
    return std::max(0, bite_tick_ - cast_tick_);  // approximate
}

int FishingSystem::GetReelPos() const { return reel_pos_; }
int FishingSystem::GetReelGreenStart() const {
    return 45 - rod_level_ * 5;  // Lv1:40, Lv5:20 — wider green zone for high rods
}
int FishingSystem::GetReelGreenEnd() const {
    return 55 + rod_level_ * 5;  // Lv1:60, Lv5:80
}

int FishingSystem::RodUpgradeCost() const {
    switch (rod_level_) { case 1: return 100; case 2: return 300; case 3: return 500; case 4: return 1000; default: return 0; }
}

bool FishingSystem::UpgradeRod() {
    if (rod_level_ >= 5) return false;
    ++rod_level_;
    return true;
}

int FishingSystem::RollFish(int current_tick) const {
    int seed = current_tick * 73 + rod_level_ * 31;
    // Actual day/night: day starts at 6am, night = hour < 6 || hour >= 18
    int tick_of_day = current_tick % kTicksPerDay;
    int game_hour = (tick_of_day * kGameMinutesPerTick / 60 + 6) % 24;
    bool is_night = (game_hour >= 18 || game_hour < 6);
    struct Candidate { const FishDef* fish; int weight; };
    std::vector<Candidate> candidates;
    for (const FishDef& f : kFishDefs) {
        if (f.night_only && !is_night) continue;
        int weight = 100;
        if (f.rarity == FishRarity::Rare) weight = 30 + rod_level_ * 10;
        else if (f.rarity == FishRarity::Legendary) weight = 5 + rod_level_ * 5;
        if (is_night && f.rarity >= FishRarity::Rare) weight += 15;
        if (f.night_only && is_night) weight += 20;
        candidates.push_back({&f, weight});
    }
    if (candidates.empty()) return kFishDefs[0].id;
    int total = 0;
    for (const Candidate& c : candidates) total += c.weight;
    int roll = (seed * 37 + 11) % total;
    int accum = 0;
    for (const Candidate& c : candidates) {
        accum += c.weight;
        if (roll < accum) return c.fish->id;
    }
    return candidates.back().fish->id;
}

bool FishingSystem::TryReelIn(int current_tick) {
    if (state_ != FishingState::Biting) return false;
    int pos = reel_pos_;
    int gs = GetReelGreenStart(), ge = GetReelGreenEnd();
    bool success = (pos >= gs && pos <= ge);
    if (success) {
        int fish_id = target_fish_id_ >= 0 ? target_fish_id_ : RollFish(current_tick);
        last_fish_id_ = fish_id;
        RecordCatch(fish_id);
    } else {
        last_fish_id_ = -1;
    }
    state_ = FishingState::Idle;
    target_fish_id_ = -1;
    reel_pos_ = 0;
    return success;
}

void FishingSystem::RecordCatch(int fish_id) {
    ++total_catches_;
    for (FishRecord& r : collection_) {
        if (r.fish_id == fish_id) { ++r.count; return; }
    }
    collection_.push_back({fish_id, 1, 0});
}

void FishingSystem::Tick(int current_tick) {
    // Waiting → Biting
    if (state_ == FishingState::Waiting && current_tick >= bite_tick_) {
        state_ = FishingState::Biting;
        reel_pos_ = 0;
        reel_speed_ = 4 - rod_level_ / 2;  // Lv1:3, Lv5:2 (slower for higher rods)
        if (reel_speed_ < 1) reel_speed_ = 1;
    }
    // Biting phase: animate reel pointer
    if (state_ == FishingState::Biting) {
        reel_pos_ += reel_speed_;
        if (reel_pos_ >= 100) {
            // Fish got away — pointer went past end
            state_ = FishingState::Idle;
            target_fish_id_ = -1;
            reel_pos_ = 0;
        }
    }
}

void FishingSystem::TickFishing(int current_tick) {
    Tick(current_tick);
}

// --- Save / Load ---
void FishingSystem::ClearForLoad() {
    Init();
}

void FishingSystem::SetForLoad(int bait, int rod, int catches,
                                const std::vector<FishRecord>& col) {
    bait_count_ = bait;
    rod_level_ = rod;
    total_catches_ = catches;
    collection_ = col;
    state_ = FishingState::Idle;
    last_fish_id_ = -1;
}

}  // namespace farm
