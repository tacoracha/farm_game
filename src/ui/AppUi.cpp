#include "ui/AppUi.h"

#include "ui/panels/CoopPanel.h"
#include "ui/panels/FarmPanel.h"
#include "ui/panels/SidebarPanel.h"
#include "ui/panels/TopStatusBar.h"
#include "ui/panels/WorkshopPanel.h"

#include <imgui.h>

#include <algorithm>

namespace farm::ui {

void AppUi::push_log(std::string line) {
    log_lines_.push_back(std::move(line));
    while (log_lines_.size() > kMaxLogLines) {
        log_lines_.pop_front();
    }
}

void AppUi::set_auto_tick_running(bool running) { auto_tick_running_ = running; }

void AppUi::set_auto_tick_speed(int multiplier) {
    if (multiplier == 2 || multiplier == 4) {
        auto_tick_speed_ = multiplier;
    } else {
        auto_tick_speed_ = 1;
    }
}

void AppUi::update_auto_tick(float delta_seconds) {
    if (!auto_tick_running_) {
        return;
    }
    const float base_seconds_per_game_tick = 0.45F;
    const float speed = static_cast<float>(std::max(1, auto_tick_speed_));
    const float interval = base_seconds_per_game_tick / speed;
    auto_tick_accumulator_ += delta_seconds;
    while (auto_tick_accumulator_ >= interval) {
        auto_tick_accumulator_ -= interval;
        game_.AdvanceTick();
    }
}

void AppUi::render_frame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin(
        "Farm Game",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    DrawTopStatusBar(*this);

    const float body_h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("body_row", ImVec2(0, body_h), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

    const float split_h = ImGui::GetContentRegionAvail().y;
    const float left_w = ImGui::GetContentRegionAvail().x * 0.62F;
    ImGui::BeginChild("left_main", ImVec2(left_w, split_h), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
    DrawFarmPanel(*this);
    DrawWorkshopPanel(*this);
    DrawCoopPanel(*this);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("right_sidebar", ImVec2(0, split_h), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
    DrawSidebarPanel(*this);
    ImGui::EndChild();

    ImGui::EndChild();

    ImGui::End();
}

}  // namespace farm::ui
