# farm_game — lightweight architecture notes

## Resource flow

- **Gameplay code** should change the player’s **gold** and **warehouse** through **`PlayerState`** (and its methods such as `TrySpendGold`, `TryAddToWarehouse`, `TryRemoveFromWarehouse`, `TrySellFromWarehouse`).
- **`Warehouse`** is the low-level stacked inventory container. Subsystems should not attach directly to a standalone `Warehouse` for the live player; they receive **`PlayerState&`** for mutations.
- **Unit tests** may construct **`Warehouse`** directly to validate inventory math in isolation; that is a test convenience, not the recommended player-facing path.

## Simulation time

- **`Game::current_tick_`** is advanced only in **`Game::AdvanceTick()`**. Subsystems should use the **`current_tick`** passed into APIs or **`Tick(current_tick)`**, not their own wall clocks.

See comments on **`Game::AdvanceTick()`** for the Tick ordering contract.
