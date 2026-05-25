#pragma once

#include <imgui.h>

namespace farm::ui {

// Rounded bordered region with a title row; pairs with EndSectionCard().
void BeginSectionCard(const char* title_id, const char* title_text);
void EndSectionCard();

// Collapsible sub-section inside a card; returns true when expanded.
// Usage: if (BeginSubSection("id", "Title")) { ... } EndSubSection();
bool BeginSubSection(const char* sub_id, const char* title_text);
void EndSubSection();

// Secondary (outline) button — PopStyleColor x4 after use.
void PushSecondaryButtonStyle();
void PopSecondaryButtonStyle();

}  // namespace farm::ui
