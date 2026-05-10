#include "ui/panels/CoopPanel.h"

#include "ui/AppUi.h"
#include "ui/UiWidgets.h"

#include "farm/ChickenCoop.h"
#include "farm/Game.h"
#include "farm/Types.h"

#include <imgui.h>

#include <sstream>
#include <string>

namespace farm::ui {

namespace {

const char* AnimalStateLabel(farm::AnimalState s) {
    switch (s) {
        case farm::AnimalState::Idle:
            return "Idle";
        case farm::AnimalState::Producing:
            return "Producing egg";
        case farm::AnimalState::Ready:
            return "Egg ready";
    }
    return "?";
}

}  // namespace

void DrawCoopPanel(AppUi& app) {
    farm::Game& g = app.game();
    farm::ChickenCoop& coop = g.Ranch().GetChickenCoop();

    BeginSectionCard("coop_card", "Chicken coop");
    ImGui::TextUnformatted("Feed 1 chicken feed to start production (game ticks).");
    ImGui::Spacing();

    const int n = coop.GetSlotCount();
    const float slot_w = (ImGui::GetContentRegionAvail().x - static_cast<float>(n - 1) * 8.0f) /
                         static_cast<float>(n > 0 ? n : 1);

    for (int i = 0; i < n; ++i) {
        if (i > 0) {
            ImGui::SameLine(0, 8);
        }
        ImGui::PushID(i);
        ImGui::BeginChild("slot", ImVec2(slot_w, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

        std::ostringstream title;
        title << "Slot " << (i + 1);
        ImGui::TextUnformatted(title.str().c_str());

        const farm::AnimalState st = coop.GetState(i);
        ImGui::TextUnformatted(AnimalStateLabel(st));

        if (st == farm::AnimalState::Idle) {
            if (ImGui::Button("Feed")) {
                auto r = coop.FeedChicken(g.Player(), i, g.CurrentTick());
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Coop slot " << i << " feed";
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
        } else if (st == farm::AnimalState::Ready) {
            if (ImGui::Button("Collect egg")) {
                auto r = coop.CollectEgg(g.Player(), i);
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Coop slot " << i << " collect";
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
        } else {
            ImGui::TextUnformatted("Wait for ticks...");
        }

        ImGui::EndChild();
        ImGui::PopID();
    }

    EndSectionCard();
}

}  // namespace farm::ui
