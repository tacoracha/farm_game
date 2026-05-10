#include "ui/UiStyle.h"

#include <imgui.h>

namespace farm::ui {

namespace {

// Reference: soft off-white shell, white cards, forest-green primary actions.
constexpr ImVec4 kBgWindow{0.94f, 0.94f, 0.93f, 1.0f};
constexpr ImVec4 kBgChild{0.99f, 0.99f, 0.98f, 1.0f};
constexpr ImVec4 kBorder{0.78f, 0.78f, 0.76f, 1.0f};
constexpr ImVec4 kTextDim{0.35f, 0.36f, 0.34f, 1.0f};
constexpr ImVec4 kPrimary{0.18f, 0.52f, 0.35f, 1.0f};
constexpr ImVec4 kPrimaryHover{0.22f, 0.60f, 0.40f, 1.0f};
constexpr ImVec4 kPrimaryActive{0.14f, 0.44f, 0.30f, 1.0f};
constexpr ImVec4 kSecondary{0.96f, 0.96f, 0.95f, 1.0f};
constexpr ImVec4 kSecondaryBorder{0.62f, 0.62f, 0.60f, 1.0f};

}  // namespace

void ApplyFarmDashboardStyle(ImGuiIO& io) {
    (void)io;
    ImGui::StyleColorsLight();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(14, 12);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(8, 6);
    s.IndentSpacing = 18.0f;
    s.ScrollbarSize = 14.0f;
    s.WindowRounding = 0.0f;
    s.ChildRounding = 8.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 6.0f;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.12f, 0.13f, 0.12f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.54f, 1.0f);
    c[ImGuiCol_WindowBg] = kBgWindow;
    c[ImGuiCol_ChildBg] = kBgChild;
    c[ImGuiCol_Border] = kBorder;
    c[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.96f, 0.98f, 0.96f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.96f, 0.93f, 1.0f);
    c[ImGuiCol_Button] = kPrimary;
    c[ImGuiCol_ButtonHovered] = kPrimaryHover;
    c[ImGuiCol_ButtonActive] = kPrimaryActive;
    c[ImGuiCol_Header] = ImVec4(0.88f, 0.92f, 0.89f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.82f, 0.90f, 0.85f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.86f, 0.80f, 1.0f);
    c[ImGuiCol_Separator] = kBorder;
    c[ImGuiCol_TitleBg] = ImVec4(0.90f, 0.91f, 0.90f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.88f, 0.90f, 0.88f, 1.0f);

    (void)kTextDim;
    (void)kSecondary;
    (void)kSecondaryBorder;
}

}  // namespace farm::ui
