#include "farm/Warehouse.h"

namespace farm {

Warehouse::Warehouse(int max_capacity) : max_capacity_(max_capacity) {
    if (max_capacity_ < 0) {
        max_capacity_ = 0;
    }
}

Result<void> Warehouse::TryAdd(ItemId id, int quantity) {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    const int current = GetCount(id);
    const int next_total = used_slots_ - current + (current + quantity);
    if (next_total > max_capacity_) {
        return Result<void>::failure(ErrorCode::WarehouseFull);
    }
    stacks_[id] = current + quantity;
    used_slots_ = next_total;
    return Result<void>::success();
}

Result<void> Warehouse::TryRemove(ItemId id, int quantity) {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    const int current = GetCount(id);
    if (current < quantity) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }
    const int remaining = current - quantity;
    if (remaining == 0) {
        stacks_.erase(id);
    } else {
        stacks_[id] = remaining;
    }
    used_slots_ -= quantity;
    return Result<void>::success();
}

bool Warehouse::HasItem(ItemId id, int minimum_count) const {
    if (minimum_count <= 0) {
        return true;
    }
    const auto it = stacks_.find(id);
    if (it == stacks_.end()) {
        return false;
    }
    return it->second >= minimum_count;
}

int Warehouse::GetCount(ItemId id) const {
    const auto it = stacks_.find(id);
    if (it == stacks_.end()) {
        return 0;
    }
    return it->second;
}

void Warehouse::SetItemProtected(ItemId id, bool protect) {
    if (protect) {
        protected_.insert(id);
    } else {
        protected_.erase(id);
    }
}

bool Warehouse::IsItemProtected(ItemId id) const { return protected_.count(id) != 0; }

Result<int> Warehouse::TrySell(ItemId id, int quantity, int unit_price) {
    if (quantity <= 0) {
        return Result<int>::failure(ErrorCode::InvalidQuantity);
    }
    if (IsItemProtected(id)) {
        return Result<int>::failure(ErrorCode::ProtectedItem);
    }
    if (unit_price <= 0) {
        return Result<int>::failure(ErrorCode::CannotSell);
    }
    auto removed = TryRemove(id, quantity);
    if (!removed.ok()) {
        return Result<int>::failure(removed.code);
    }
    return Result<int>::success(quantity * unit_price);
}

}  // namespace farm
