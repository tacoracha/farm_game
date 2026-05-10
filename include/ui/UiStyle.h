#pragma once

struct ImGuiIO;

namespace farm::ui {

// Light dashboard palette + spacing tuned for a management-style layout (not default ImGui demo).
void ApplyFarmDashboardStyle(ImGuiIO& io);

}  // namespace farm::ui
