#include "ui/panels/WorkshopPanel.h"

#include "ui/AppUi.h"
#include "ui/UiWidgets.h"

#include "farm/FeedMill.h"
#include "farm/Game.h"
#include "farm/Types.h"

#include <imgui.h>

#include <sstream>
#include <string>

namespace farm::ui {

void DrawWorkshopPanel(AppUi& app) {
    farm::Game& g = app.game();
    farm::FeedMill& mill = g.Workshop().GetFeedMill();

    BeginSectionCard("workshop_card", "Workshop (Feed mill)");
    ImGui::TextUnformatted("Recipes: 2 Wheat -> Chicken feed; 2 Corn + 1 Carrot -> Cow feed");
    ImGui::Spacing();

    {
        std::ostringstream oss;
        mill.WriteStatus(oss);
        const std::string status = oss.str();
        ImGui::TextWrapped("%s", status.c_str());
    }

    ImGui::Spacing();

    if (mill.IsIdle()) {
        if (ImGui::Button("Start chicken feed")) {
            auto r = mill.StartProduction(g.Player(), farm::RecipeId::ChickenFeed, g.CurrentTick());
            std::ostringstream log;
            log << "[tick " << g.CurrentTick() << "] Feed mill start chicken feed";
            if (!r.ok()) {
                log << " -> " << farm::ErrorCodeMessage(r.code);
            }
            app.push_log(log.str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Start cow feed")) {
            auto r = mill.StartProduction(g.Player(), farm::RecipeId::CowFeed, g.CurrentTick());
            std::ostringstream log;
            log << "[tick " << g.CurrentTick() << "] Feed mill start cow feed";
            if (!r.ok()) {
                log << " -> " << farm::ErrorCodeMessage(r.code);
            }
            app.push_log(log.str());
        }
    } else if (mill.IsReadyToCollect()) {
        if (ImGui::Button("Collect output")) {
            auto r = mill.Collect(g.Player());
            std::ostringstream log;
            log << "[tick " << g.CurrentTick() << "] Feed mill collect";
            if (!r.ok()) {
                log << " -> " << farm::ErrorCodeMessage(r.code);
            }
            app.push_log(log.str());
        }
    } else {
        ImGui::TextUnformatted("Production in progress (advances with game ticks).");
    }

    EndSectionCard();
}

}  // namespace farm::ui
