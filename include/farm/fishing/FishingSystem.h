#pragma once

#include "farm/common/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace farm {

class PlayerState;

enum class FishRarity : std::uint8_t { Common, Rare, Legendary };
enum class FishingState : std::uint8_t { Idle, Waiting, Biting, ReeledIn };

struct FishDef {
    int id = 0;
    const char* name = "";
    FishRarity rarity = FishRarity::Common;
    int sell_price = 5;
    Season season_mask = static_cast<Season>(0xF);  // bitmask: all seasons
    bool night_only = false;
    int difficulty = 50;  // 0-100, higher = harder to catch
};

struct FishRecord {
    int fish_id = 0;
    int count = 0;
    int max_size = 0;
};

class FishingSystem {
public:
    FishingSystem();

    void Init();
    void Tick(int current_tick);

    // Fishing state machine
    bool CanFish() const;
    void CastLine(int current_tick);
    void TickFishing(int current_tick);
    int GetBiteCountdown() const;       // ticks until bite
    bool IsBiting() const { return state_ == FishingState::Biting; }
    const FishDef* GetPendingFish() const;

    // Reel mini-game
    int GetReelPos() const;            // 0-100, moving pointer position
    int GetReelGreenStart() const;     // green zone start
    int GetReelGreenEnd() const;       // green zone end
    Result<void> TryReelIn(int current_tick);  // returns Result with specific error on failure
    FishingState State() const { return state_; }

    // Bait & rod
    int BaitCount() const { return bait_count_; }
    void AddBait(int n) { bait_count_ += n; }
    bool UseBait();
    int RodLevel() const { return rod_level_; }
    int RodUpgradeCost() const;
    Result<void> UpgradeRod();

    // Fish info
    static const std::vector<FishDef>& FishList();
    const FishDef* GetFishDef(int id) const;

    // Collection
    const std::vector<FishRecord>& Collection() const { return collection_; }
    int TotalCatches() const { return total_catches_; }
    void RecordCatch(int fish_id);

    // Last catch
    int LastCaughtFish() const { return last_fish_id_; }

    // Save / Load
    void ClearForLoad();
    void SetForLoad(int bait, int rod, int catches, const std::vector<FishRecord>& col);

    int BaitForSave() const { return bait_count_; }
    int RodForSave() const { return rod_level_; }
    int CatchesForSave() const { return total_catches_; }
    const std::vector<FishRecord>& CollectionForSave() const { return collection_; }

private:
    int RollFish(int current_tick) const;

    int bait_count_ = 0;
    int rod_level_ = 1;          // 1=basic, 2=advanced, 3=master
    int total_catches_ = 0;
    int last_fish_id_ = -1;

    FishingState state_ = FishingState::Idle;
    int cast_tick_ = 0;
    int bite_tick_ = 0;
    int target_fish_id_ = -1;
    int reel_pos_ = 0;            // mini-game pointer 0-100
    int reel_speed_ = 3;          // pointer movement speed
    int reel_green_start_ = 35;
    int reel_green_end_ = 65;

    std::vector<FishRecord> collection_;
};

}  // namespace farm
