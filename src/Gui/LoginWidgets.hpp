#pragma once
#include <Gui/UI.hpp>

namespace LoginUI
{
    // Local to authentication: preserve the project's other custom buttons.
    inline bool AnimatedButton(const char* label, const ImVec2& size)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;
        const ImGuiID id = window->GetID(label);
        const ImRect bounds(window->DC.CursorPos, window->DC.CursorPos + size);
        ImGui::ItemSize(size);
        if (!ImGui::ItemAdd(bounds, id)) return false;
        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(bounds, id, &hovered, &held);
        static std::map<ImGuiID, float> hover;
        float& blend = hover[id];
        blend = ImLerp(blend, hovered ? 1.f : 0.f, UI::MotionStep(14.f));
        ImVec4 fill = ImLerp(UI::Surface2(), ImVec4(.17f, .145f, .025f, 1.f), blend);
        if (held) fill = ImVec4(.23f, .19f, .025f, 1.f);
        window->DrawList->AddRectFilled(bounds.Min, bounds.Max, ImGui::GetColorU32(fill), 6.f);
        window->DrawList->AddRect(bounds.Min, bounds.Max,
            ImGui::GetColorU32(ImLerp(UI::Border(), UI::Accent(), blend)), 6.f);
        if (blend > .01f) {
            ImVec4 glow = UI::Accent();
            glow.w = .12f * blend;
            window->DrawList->AddRect(bounds.Min + ImVec2(1, 1), bounds.Max - ImVec2(1, 1),
                ImGui::GetColorU32(glow), 6.f, 0, 2.f);
        }
        ImGui::RenderNavHighlight(bounds, id);
        ImGui::PushStyleColor(ImGuiCol_Text, ImLerp(UI::Text(), UI::Accent(), blend));
        const float lift = UI::ReduceMotion ? 0.f : blend;
        ImGui::RenderTextClipped(bounds.Min - ImVec2(0, lift), bounds.Max - ImVec2(0, lift),
            label, ImGui::FindRenderedTextEnd(label), nullptr, ImVec2(.5f, .5f));
        ImGui::PopStyleColor();
        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        return pressed;
    }
}
