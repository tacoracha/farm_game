#include "ui/panels/SidebarPanel.h"

#include "ui/AppUi.h"
#include "ui/UiWidgets.h"

#include "farm/Game.h"
#include "farm/ItemCatalog.h"
#include "farm/OrderSystem.h"
#include "farm/PlayerState.h"
#include "farm/ShopSystem.h"
#include "farm/Types.h"
#include "farm/Warehouse.h"

#include <imgui.h>

#include <algorithm>
#include <sstream>
#include <string>

namespace farm::ui {

namespace {

constexpr farm::ItemId kAllItems[] = {
    farm::ItemId::WheatSeed, farm::ItemId::CornSeed, farm::ItemId::CarrotSeed,
    farm::ItemId::Wheat,    farm::ItemId::Corn,     farm::ItemId::Carrot,
    farm::ItemId::ChickenFeed, farm::ItemId::CowFeed, farm::ItemId::Egg,
};

void DrawWarehouseSection(AppUi& app) {
    farm::Game& g = app.game();
    farm::PlayerState& p = g.Player();
    const farm::Warehouse& wh = p.GetWarehouse();

    BeginSectionCard("warehouse_card", "Warehouse");
    if (ImGui::BeginTable("warehouse_tbl", 4,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthStretch, 0.15f);
        ImGui::TableSetupColumn("Sell", ImGuiTableColumnFlags_WidthStretch, 0.20f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.20f);
        ImGui::TableHeadersRow();

        for (farm::ItemId id : kAllItems) {
            const int qty = p.GetItemCount(id);
            if (qty <= 0) {
                continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(farm::ItemCatalog::Get(id).name.data());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", qty);

            const int unit = farm::ItemCatalog::SellPrice(id);
            ImGui::TableSetColumnIndex(2);
            if (unit > 0) {
                ImGui::Text("%d g / unit", unit);
            } else {
                ImGui::TextUnformatted("-");
            }

            ImGui::TableSetColumnIndex(3);
            PushSecondaryButtonStyle();
            std::ostringstream sell_id;
            sell_id << "Sell x1##sell_" << static_cast<int>(id);
            const bool can_sell = unit > 0 && qty > 0;
            if (!can_sell) {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton(sell_id.str().c_str())) {
                auto r = p.TrySellFromWarehouse(id, 1);
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Sell " << farm::ItemCatalog::Get(id).name.data()
                    << " x1";
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
            if (!can_sell) {
                ImGui::EndDisabled();
            }
            PopSecondaryButtonStyle();
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Text("Capacity: %d / %d", wh.UsedSlots(), wh.MaxCapacity());
    EndSectionCard();
}

void DrawSeedShopSection(AppUi& app) {
    farm::Game& g = app.game();
    farm::PlayerState& p = g.Player();
    farm::ShopSystem& shop = g.Shop();

    BeginSectionCard("seed_shop_card", "Seed shop");
    if (ImGui::BeginTable("seed_tbl", 3,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Seed", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Buy", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableHeadersRow();

        for (farm::ItemId id : kAllItems) {
            if (!shop.IsPurchasableSeed(id)) {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(farm::ItemCatalog::Get(id).name.data());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d g", shop.GetSeedPrice(id));

            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton((std::string("Buy x1##buy_") + std::to_string(static_cast<int>(id))).c_str())) {
                auto r = shop.BuySeed(p, id, 1);
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Buy seed " << farm::ItemCatalog::Get(id).name.data();
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
        }

        ImGui::EndTable();
    }
    EndSectionCard();
}

void DrawOrdersSection(AppUi& app) {
    farm::Game& g = app.game();
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    BeginSectionCard("orders_card", "Orders");
    const auto& slots = orders.GetOrders();
    for (std::size_t i = 0; i < slots.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const farm::Order& o = slots[i];
        ImGui::BeginChild("order_slot", ImVec2(-1, 0),
                          ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

        ImGui::Text("%s x%d", farm::ItemCatalog::Get(o.item_id).name.data(), o.quantity);
        ImGui::Text("Reward: %d gold", o.reward_gold);
        const int have = p.GetItemCount(o.item_id);
        ImGui::Text("In warehouse: %d / %d", have, o.quantity);

        if (ImGui::Button("Complete order")) {
            auto r = orders.CompleteOrder(p, static_cast<int>(i));
            std::ostringstream log;
            log << "[tick " << g.CurrentTick() << "] Complete order slot " << i;
            if (!r.ok()) {
                log << " -> " << farm::ErrorCodeMessage(r.code);
            }
            app.push_log(log.str());
        }

        ImGui::EndChild();
        ImGui::Spacing();
        ImGui::PopID();
    }
    EndSectionCard();
}

}  // namespace

void DrawSidebarPanel(AppUi& app) {
    DrawWarehouseSection(app);
    DrawSeedShopSection(app);
    DrawOrdersSection(app);

    ImGui::Separator();
    ImGui::TextUnformatted("Log");
    float log_h = ImGui::GetContentRegionAvail().y - 6.0f;
    log_h = std::max(110.0f, log_h);
    ImGui::BeginChild("log_region", ImVec2(-1, log_h), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const std::string& line : app.log_lines()) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (!app.log_lines().empty()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

}  // namespace farm::ui
