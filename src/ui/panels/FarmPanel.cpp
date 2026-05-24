#include "ui/panels/FarmPanel.h"

#include "ui/AppUi.h"
#include "ui/UiWidgets.h"

#include "farm/Game.h"
#include "farm/ItemCatalog.h"
#include "farm/PlantingSystem.h"
#include "farm/Types.h"

#include <imgui.h>

#include <algorithm>
#include <iterator>
#include <sstream>

namespace farm::ui {

namespace {

const char* PlotStateShort(farm::PlotState s) {
    switch (s) {
        case farm::PlotState::Idle:
            return "Idle";
        case farm::PlotState::Growing:
            return "Growing";
        case farm::PlotState::Mature:
            return "Ready";
    }
    return "?";
}

struct QuickSeed {
    farm::ItemId id;
    const char* label;
};

constexpr QuickSeed kQuickSeeds[] = {
    {farm::ItemId::WheatSeed, "Wheat"},
    {farm::ItemId::CornSeed, "Corn"},
    {farm::ItemId::CarrotSeed, "Carrot"},
};

}  // namespace

void DrawFarmPanel(AppUi& app) {
    farm::Game& g = app.game();
    const auto& plots = g.Planting().GetPlotsForDisplay();
    const int tick_now = g.CurrentTick();

    BeginSectionCard("farm_card", "Farm (plots)");
    ImGui::TextUnformatted("Each plot is independent. Planting uses the selected plot.");
    ImGui::Spacing();

    const int n = static_cast<int>(plots.size());
    const float card_w =
        (ImGui::GetContentRegionAvail().x - static_cast<float>(std::max(0, n - 1)) * 8.0f) /
        static_cast<float>(n > 0 ? n : 1);

    for (int i = 0; i < n; ++i) {
        if (i > 0) {
            ImGui::SameLine(0, 8);
        }

        ImGui::PushID(i);
        ImGui::BeginChild("plot_card", ImVec2(card_w, 0),
                           ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

        std::ostringstream hdr;
        hdr << "Plot " << (i + 1);
        ImGui::TextUnformatted(hdr.str().c_str());

        const farm::PlotView plot = plots[static_cast<std::size_t>(i)];
        ImGui::TextUnformatted(PlotStateShort(plot.state));

        if (plot.state == farm::PlotState::Growing) {
            const char* crop_name = farm::ItemCatalog::Get(plot.crop_id).name.data();
            const int remaining = std::max(0, plot.finish_tick - tick_now);
            ImGui::Text("Crop: %s", crop_name);
            ImGui::Text("Ticks left: %d", remaining);
            ImGui::Text("Water: %s", plot.water_state == farm::PlotWaterState::Watered ? "Watered" : "Dry");
            ImGui::Text("Fertilizer: %s", plot.fertilized ? "Used" : "No");

            if (plot.water_state == farm::PlotWaterState::Watered) {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton("Water")) {
                auto r = g.Planting().WaterPlot(i, g.CurrentTick());
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Water plot " << i;
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
            if (plot.water_state == farm::PlotWaterState::Watered) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (plot.fertilized || g.Player().GetItemCount(farm::ItemId::Fertilizer) <= 0) {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton("Fertilize")) {
                auto r = g.Planting().ApplyFertilizer(g.Player(), i, farm::ItemId::Fertilizer);
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Fertilize plot " << i;
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
            if (plot.fertilized || g.Player().GetItemCount(farm::ItemId::Fertilizer) <= 0) {
                ImGui::EndDisabled();
            }
        } else if (plot.state == farm::PlotState::Mature) {
            const char* crop_name = farm::ItemCatalog::Get(plot.crop_id).name.data();
            ImGui::Text("Crop: %s", crop_name);
            if (ImGui::Button("Harvest")) {
                auto r = g.Planting().TryHarvest(g.Player(), i);
                std::ostringstream log;
                log << "[tick " << g.CurrentTick() << "] Harvest plot " << i;
                if (!r.ok()) {
                    log << " -> " << farm::ErrorCodeMessage(r.code);
                }
                app.push_log(log.str());
            }
        } else {
            ImGui::TextUnformatted("Choose a crop:");
            for (std::size_t si = 0; si < std::size(kQuickSeeds); ++si) {
                const QuickSeed& qs = kQuickSeeds[si];
                if (si > 0) {
                    ImGui::SameLine();
                }
                if (ImGui::SmallButton(qs.label)) {
                    auto r = g.Planting().TryPlantAt(g.Player(), i, qs.id, g.CurrentTick());
                    std::ostringstream log;
                    log << "[tick " << g.CurrentTick() << "] Plant " << qs.label << " plot " << i;
                    if (!r.ok()) {
                        log << " -> " << farm::ErrorCodeMessage(r.code);
                    }
                    app.push_log(log.str());
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopID();
    }

    EndSectionCard();
}

}  // namespace farm::ui
