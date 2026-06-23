#pragma once

#include "farm/common/Constants.h"
#include "farm/common/Types.h"

#include <map>
#include <set>
#include <vector>

namespace farm {

class AchievementSystem;

struct InventoryItemView {
    ItemId item = ItemId::Wheat;
    int quantity = 0;
    bool locked = false;
};

class PlayerState {
public:
    PlayerState();

    int Gold() const { return gold_; }
    int Level() const { return level_; }
    int Experience() const { return experience_; }
    int ExpToNextLevel() const { return kExpPerLevel; }
    int WarehouseCapacity() const { return warehouse_capacity_; }
    int WarehouseUsed() const;
    int WarehouseRemaining() const { return warehouse_capacity_ - WarehouseUsed(); }
    int WarehouseLevel() const { return warehouse_level_; }

    bool HasItem(ItemId item, int quantity) const;
    int ItemCount(ItemId item) const;
    bool IsItemLocked(ItemId item) const;
    bool IsSeedUnlocked(ItemId seed) const;
    bool IsUnlocked(UnlockId id) const;
    bool CanUnlock(UnlockId id) const;
    std::vector<InventoryItemView> InventoryView() const;

    Result<void> TryAddItem(ItemId item, int quantity);
    Result<void> TryRemoveItem(ItemId item, int quantity);
    Result<void> TryRemoveItems(const std::vector<ItemStack>& items);
    Result<void> TrySpendGold(int amount);
    Result<void> TrySellItem(ItemId item, int quantity);
    int BatchSellAll();
    Result<void> UpgradeWarehouse();
    Result<void> UnlockContent(UnlockId id);

    void AddGold(int amount);
    void AddExperience(int amount);
    void SetItemLocked(ItemId item, bool locked);

    // Called by Game after construction to wire up event systems
    void Setup(AchievementSystem* ach);

    // Save/Load — called by SaveManager via Game
    void ClearForLoad();
    void SetGoldForLoad(int gold) { gold_ = gold; }
    void SetLevelForLoad(int level) { level_ = level; }
    void SetExperienceForLoad(int experience) { experience_ = experience; }
    void SetWarehouseLevelForLoad(int level) { warehouse_level_ = level; }
    void SetWarehouseCapacityForLoad(int capacity) { warehouse_capacity_ = capacity; }
    void SetItemForLoad(ItemId item, int quantity);
    void SetSeedUnlockedForLoad(ItemId seed, bool unlocked);
    void SetUnlockedForLoad(UnlockId id, bool unlocked);
    const std::set<UnlockId>& UnlockedContentForSave() const { return unlocked_content_; }

private:
    int gold_ = kInitialGold;
    int level_ = 1;
    int experience_ = 0;
    int warehouse_level_ = 1;
    int warehouse_capacity_ = kInitialWarehouseCapacity;
    std::map<ItemId, int> items_;
    std::set<ItemId> locked_items_;
    std::set<ItemId> unlocked_seeds_;
    std::set<UnlockId> unlocked_content_;

    // Event system pointer (non-owning)
    AchievementSystem* ach_ = nullptr;
};

}  // namespace farm
