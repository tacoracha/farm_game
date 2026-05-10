#include "ui/panels/TopStatusBar.h"

#include "ui/AppUi.h"
#include "ui/UiWidgets.h"

#include "farm/Game.h"
#include "farm/PlayerState.h"
#include "farm/Warehouse.h"

#include <imgui.h>

#include <sstream>

namespace farm::ui {

void DrawTopStatusBar(AppUi& app) {
    farm::Game& g = app.game();
    const farm::PlayerState& p = g.Player();
    const farm::Warehouse& wh = p.GetWarehouse();
    const int tick = g.CurrentTick();

    const float bar_h = ImGui::GetFrameHeightWithSpacing() * 4.2f;
    ImGui::BeginChild("top_status_bar", ImVec2(0, bar_h), ImGuiChildFlags_Border,
                      ImGuiWindowFlags_NoScrollbar);

    // Row 1 — resource strip (dashboard readout)
    ImGui::TextUnformatted("Gold");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.10f, 0.12f, 0.10f, 1.0f), "%d", p.Gold());
    ImGui::SameLine(0, 22);
    ImGui::TextUnformatted("Tick");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.10f, 0.12f, 0.10f, 1.0f), "%d", tick);
    ImGui::SameLine(0, 22);
    ImGui::TextUnformatted("Warehouse");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.10f, 0.12f, 0.10f, 1.0f), "%d / %d", wh.UsedSlots(),
                       wh.MaxCapacity());

    ImGui::Spacing();

    // Row 2 — simulation controls
    const char* auto_label = app.auto_tick_running() ? "Auto: running" : "Auto: paused";
    ImGui::TextUnformatted(auto_label);
    ImGui::SameLine();
    if (ImGui::Button("Start##auto")) {
        app.set_auto_tick_running(true);
    }
    ImGui::SameLine();
    PushSecondaryButtonStyle();
    if (ImGui::Button("Pause##auto")) {
        app.set_auto_tick_running(false);
    }
    PopSecondaryButtonStyle();
    ImGui::SameLine(0, 14);
    ImGui::TextUnformatted("Speed");
    ImGui::SameLine();
    for (int m : {1, 2, 4}) {
        ImGui::SameLine();
        const bool on = (app.auto_tick_speed() == m);
        if (on) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.60f, 0.40f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.44f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        std::ostringstream tag;
        tag << m << "x##speed";
        if (ImGui::Button(tag.str().c_str())) {
            app.set_auto_tick_speed(m);
        }
        if (on) {
            ImGui::PopStyleColor(4);
        }
    }

    ImGui::SameLine(0, 18);
    if (ImGui::Button("Advance 1 Tick")) {
        g.AdvanceTick();
        std::ostringstream oss;
        oss << "[tick " << g.CurrentTick() << "] Manual advance +1";
        app.push_log(oss.str());
    }
    ImGui::SameLine();
    PushSecondaryButtonStyle();
    if (ImGui::Button("Advance 3 Ticks")) {
        for (int i = 0; i < 3; ++i) {
            g.AdvanceTick();
        }
        std::ostringstream oss;
        oss << "[tick " << g.CurrentTick() << "] Manual advance +3";
        app.push_log(oss.str());
    }
    PopSecondaryButtonStyle();

    ImGui::EndChild();
}

}  // namespace farm::ui
