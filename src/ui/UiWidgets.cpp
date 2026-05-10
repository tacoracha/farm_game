#include "ui/UiWidgets.h"

namespace farm::ui {

void BeginSectionCard(const char* title_id, const char* title_text) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::BeginChild(title_id, ImVec2(-1, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::TextColored(ImVec4(0.18f, 0.19f, 0.18f, 1.0f), "%s", title_text);
    ImGui::Spacing();
}

void EndSectionCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::Spacing();
}

void PushSecondaryButtonStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.96f, 0.96f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.94f, 0.92f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.88f, 0.91f, 0.88f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.21f, 0.20f, 1.0f));
}

void PopSecondaryButtonStyle() { ImGui::PopStyleColor(4); }

}  // namespace farm::ui
