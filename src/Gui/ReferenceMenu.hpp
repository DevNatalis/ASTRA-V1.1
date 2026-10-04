#pragma once
#include <Gui/UI.hpp>

// Compact reference layout. All controls use the existing configuration fields.
namespace ReferenceMenu
{
    constexpr float Width = 880.f, Height = 460.f;
    constexpr float SidebarWidth = 230.f, Top = 36.f, Gap = 14.f;
    inline int Group = 0;
    inline int AimPage = 0;
    inline int VisualPage = 0;

    inline ImU32 Color(int r, int g, int b)
    {
        // Raw draw-list colors bake alpha: without this, menu art ignores the
        // ImGuiStyleVar_Alpha fade and stays on screen when the menu is closed.
        return ImGui::GetColorU32(ImVec4(r / 255.f, g / 255.f, b / 255.f, ImGui::GetStyle().Alpha));
    }

    inline bool Nav(const char* label, bool selected, bool child = false)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float h = 23.f;
        const bool pressed = ImGui::InvisibleButton(label, ImVec2(SidebarWidth - 26.f, h));
        ImDrawList* draw = ImGui::GetWindowDrawList();
        if (ImGui::IsItemHovered())
            draw->AddRectFilled(p, p + ImVec2(SidebarWidth - 26.f, h), Color(20, 20, 23), 3.f);
        if (selected && child)
            draw->AddCircleFilled(p + ImVec2(11, h * .5f), 2.f, ImGui::GetColorU32(UI::Accent()));
        if (selected && !child)
            draw->AddRectFilled(p + ImVec2(0, 3), p + ImVec2(2, h - 3), ImGui::GetColorU32(UI::Accent()));
        draw->AddText(p + ImVec2(child ? 24.f : 8.f, (h - ImGui::GetFontSize()) * .5f),
            selected || ImGui::IsItemHovered() ? Color(236, 236, 240) : Color(154, 154, 164), label);
        return pressed;
    }

    inline void Select(int tab, int section = 0)
    {
        g_MenuInfo.iTabCount = tab;
        g_MenuInfo.iCurrentPage = tab;
        g_MenuInfo.SubPage[tab] = section;
    }

    inline void Shell()
    {
        // Menu closed -> style alpha fades to 0. Skip raw draws entirely so
        // nothing (logo, sidebar art) lingers on screen. ESP/FOV/watermark
        // are drawn elsewhere and are unaffected.
        const float menuAlpha = ImGui::GetStyle().Alpha;
        if (menuAlpha <= 0.01f)
            return;
        const ImVec2 p = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(p, p + size, Color(13, 13, 15), 5.f);
        draw->AddRect(p, p + size, Color(31, 31, 35), 5.f);
        draw->AddLine(p + ImVec2(SidebarWidth, Top), p + ImVec2(SidebarWidth, size.y - 10), Color(23, 23, 26));
        ImFont* icon = UI::SafeFont(g_Variables.FontAwesomeSolidSmall);
        const float logoSize = 80.f;
        const ImVec2 logoPos = p + ImVec2((SidebarWidth - logoSize) * .5f, 10.f);
        if (g_Variables.Logo)
            draw->AddImage(g_Variables.Logo, logoPos, logoPos + ImVec2(logoSize, logoSize),
                ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, menuAlpha)));

        constexpr float navigationTop = 100.f;
        ImGui::SetCursorPos(ImVec2(12, navigationTop));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::BeginChild("##reference_navigation", ImVec2(SidebarWidth - 24, size.y - navigationTop - 60), false);
        if (Nav("Aim Assistance", Group == 0)) { Group = 0; Select(g_MenuInfo.Combat, AimPage); }
        if (Group == 0)
        {
            const char* names[] = { "Aim Bot", "Trigger Bot", "Silent Aim" };
            for (int i = 0; i < 3; ++i)
                if (Nav(names[i], AimPage == i, true)) { AimPage = i; Select(g_MenuInfo.Combat, i); }
        }
        if (Nav("Visuals", Group == 1)) { Group = 1; Select(VisualPage == 0 ? g_MenuInfo.Visuals : g_MenuInfo.VisualsVehicles); }
        if (Group == 1)
        {
            if (Nav("Players", VisualPage == 0, true)) { VisualPage = 0; Select(g_MenuInfo.Visuals); }
            if (Nav("Vehicles", VisualPage == 1, true)) { VisualPage = 1; Select(g_MenuInfo.VisualsVehicles); }
        }
        if (Nav("Jogador", Group == 4)) { Group = 4; Select(g_MenuInfo.Local); }
        if (Group == 4)
        {
            const char* names[] = { "Geral", "Diversos", "Teleports" };
            for (int i = 0; i < 3; ++i)
                if (Nav(names[i], g_MenuInfo.SubPage[g_MenuInfo.Local] == i, true)) Select(g_MenuInfo.Local, i);
        }
        if (Nav("Misc", Group == 2)) { Group = 2; Select(g_MenuInfo.Weapons); }
        if (Group == 2)
        {
            const char* names[] = { "Weapons", "Players list", "Vehicles list", "Vehicle controls" };
            const int tabs[] = { g_MenuInfo.Weapons, g_MenuInfo.World, g_MenuInfo.Vehicless, g_MenuInfo.VisualsVehicles };
            for (int i = 0; i < 4; ++i)
                if (Nav(names[i], g_MenuInfo.iTabCount == tabs[i], true)) Select(tabs[i], i == 3 ? 1 : 0);
        }
        if (Nav("Settings", Group == 3)) { Group = 3; Select(g_MenuInfo.Settings); }
        if (Group == 3)
        {
            const char* names[] = { "General", "Configs", "Colors" };
            for (int i = 0; i < 3; ++i)
                if (Nav(names[i], g_MenuInfo.SubPage[g_MenuInfo.Settings] == i, true)) Select(g_MenuInfo.Settings, i);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);

        const ImVec2 profile = p + ImVec2(12, size.y - 46);
        draw->AddRectFilled(profile, profile + ImVec2(SidebarWidth - 24, 36), Color(18, 18, 21), 4.f);
        draw->AddCircleFilled(profile + ImVec2(22, 18), 13, Color(93, 72, 211));
        const ImVec2 userIcon = icon->CalcTextSizeA(12, FLT_MAX, 0, ICON_FA_USER);
        draw->AddText(icon, 12, profile + ImVec2(22, 18) - userIcon * .5f, Color(255, 255, 255), ICON_FA_USER);
        draw->PushClipRect(profile + ImVec2(43, 0), profile + ImVec2(SidebarWidth - 32, 36), true);
        draw->AddText(profile + ImVec2(43, 3), Color(229, 229, 234), g_Variables.UserName.c_str());
        draw->AddText(profile + ImVec2(43, 19), Color(100, 100, 113), g_Variables.Role.c_str());
        draw->PopClipRect();
    }

    inline bool Check(const char* label, bool* value)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImGuiID id = window->GetID(label);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImRect row(p, p + ImVec2(ImGui::GetContentRegionAvail().x, 20.f));
        ImGui::ItemSize(row);
        if (!ImGui::ItemAdd(row, id)) return false;
        bool hovered, held;
        const bool pressed = ImGui::ButtonBehavior(row, id, &hovered, &held);
        if (pressed) { *value = !*value; ImGui::MarkItemEdited(id); }
        const ImVec2 box(row.Max.x - 22, row.Min.y + 2);
        window->DrawList->AddRectFilled(box, box + ImVec2(22, 16),
            *value ? ImGui::GetColorU32(UI::Accent()) : Color(19, 19, 23), 4.f);
        window->DrawList->AddRect(box, box + ImVec2(22, 16), hovered ? ImGui::GetColorU32(UI::Accent()) : Color(37, 37, 44), 4.f);
        if (*value) ImGui::RenderCheckMark(window->DrawList, box + ImVec2(5, 3), Color(20, 20, 23), 11.f);
        ImGui::RenderTextClipped(row.Min + ImVec2(0, 3), ImVec2(box.x - 7, row.Max.y), label, nullptr, nullptr);
        return pressed;
    }

    inline void InverseCheck(const char* label, bool* value)
    {
        bool enabled = !*value;
        if (Check(label, &enabled)) *value = !enabled;
    }

    inline void ColorRow(const char* label, ImColor& value)
    {
        const ImVec2 p = ImGui::GetCursorPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(p.x + width - 22, p.y));
        ImGui::PushID(label);
        if (ImGui::ColorButton("##swatch", value.Value, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(22, 16))) ImGui::OpenPopup("picker");
        if (ImGui::BeginPopup("picker"))
        {
            ImGui::ColorPicker4("##color", &value.Value.x, ImGuiColorEditFlags_AlphaBar);
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::SetCursorPosY(p.y + 22);
    }

    inline void Slider(const char* label, int* value, int min, int max, const char* format)
    {
        ImGui::SliderIntCustom(label, value, min, max, format, 0);
    }

    inline bool Panel(const char* title, const ImVec2& size)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 9));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(16/255.f, 16/255.f, 19/255.f, 1));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(24/255.f, 24/255.f, 28/255.f, 1));
        const bool visible = ImGui::BeginChild(title, size, true, ImGuiWindowFlags_AlwaysUseWindowPadding);
        const float x = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(x + ImMax(0.f, (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(title).x) * .5f));
        ImGui::TextColored(ImVec4(.46f, .46f, .50f, 1), "%s", title);
        ImGui::SetCursorPosX(x);
        ImGui::Spacing();
        return visible;
    }

    inline void EndPanel()
    {
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }

    inline ImVec2 PanelSize(float requestedHeight)
    {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        return ImVec2((available.x - Gap) * .5f, ImMin(available.y, requestedHeight));
    }

    inline void Aim()
    {
        const ImVec2 size = PanelSize(280);
        auto& config = Core::g_Config;
        bool* enabled; bool* show; bool* visible; bool* ignore;
        int* key; int* fov; int* distance; ImColor* color;
        const char* titles[] = { "AIMBOT", "TRIGGERBOT", "SILENTAIM" };
        if (AimPage == 0) {
            enabled = &config.Aimbot->Enabled; show = &config.Aimbot->ShowFov; visible = &config.Aimbot->OnlyVisible;
            ignore = &config.Aimbot->IgnoreNPCs; key = &config.Aimbot->KeyBind; fov = &config.Aimbot->FOV;
            distance = &config.Aimbot->MaxDistance; color = &config.Aimbot->FovColor;
        } else if (AimPage == 1) {
            enabled = &config.TriggerBot->Enabled; show = &config.TriggerBot->ShowFov; visible = &config.TriggerBot->OnlyVisible;
            ignore = &config.TriggerBot->IgnoreNPCs; key = &config.TriggerBot->KeyBind; fov = &config.TriggerBot->FOV;
            distance = &config.TriggerBot->MaxDistance; color = &config.TriggerBot->FovColor;
        } else {
            enabled = &config.SilentAim->Enabled; show = &config.SilentAim->ShowFov; visible = &config.SilentAim->OnlyVisible;
            ignore = &config.SilentAim->IgnoreNPCs; key = &config.SilentAim->KeyBind; fov = &config.SilentAim->FOV;
            distance = &config.SilentAim->MaxDistance; color = &config.SilentAim->FovColor;
        }
        if (Panel(titles[AimPage], size)) {
            Check("Enabled", enabled);
            static int modes[3] = { 1, 1, 1 };
            ImGui::Keybind("Bind", key, &modes[AimPage]);
            if (AimPage == 2) Check("Magic Bullet", &config.SilentAim->MagicBullets);
            if (AimPage == 1) Check("Smart Trigger", &config.TriggerBot->SmartTrigger);
            InverseCheck("Target Peds", ignore);
            Check("Visible Only", visible);
            if (AimPage != 1 || !config.TriggerBot->SmartTrigger) Check("Draw Fov", show);
            else *show = false;
            ColorRow("Fov Color", *color);
        }
        EndPanel();
        ImGui::SameLine(0, Gap);
        const std::string settings = std::string(titles[AimPage]) + " SETTINGS";
        if (Panel(settings.c_str(), size)) {
            if (AimPage != 1 || !config.TriggerBot->SmartTrigger) {
                Slider("Fov", fov, 0, 400, "%dpx");
                Slider("Max Distance", distance, 0, 1000, "%dm");
            }
            if (AimPage == 0) Slider("Smooth", &config.Aimbot->AimbotSpeed, 0, 100, "%d");
            if (AimPage == 1) Slider("Delay", &config.TriggerBot->Delay, 0, 10, "%d");
            if (AimPage == 2) Slider("Miss Chance", &config.SilentAim->MissChance, 0, 100, "%d%%");
        }
        EndPanel();
    }

    inline void Players()
    {
        const ImVec2 size = PanelSize(394);
        auto& c = Core::g_Config;
        if (Panel("VISUALS", size)) {
            Check("Enabled", &c.ESP->Enabled);
            Slider("Render Distance", &c.ESP->MaxDistance, 0, 1000, "%dm");
            Check("Select LocalPlayer", &c.ESP->ShowLocalPlayer);
            InverseCheck("Select Peds", &c.ESP->IgnoreNPCs);
            InverseCheck("Show Dead", &c.ESP->IgnoreDead);
            Check("Highlight Visible", &c.ESP->HighlightVisible);
            Check("Friends Marker", &c.ESP->FriendsMarker);
            if (c.ESP->FriendsMarker) { static int mode = 1; ImGui::Keybind("Marker Bind", &c.ESP->FriendsMarkerBind, &mode); }
            Check("Admin ESP", &c.ESP->AdminESP);
        }
        EndPanel();
        ImGui::SameLine(0, Gap);
        if (Panel("VISUALS SETTINGS", size)) {
            Check("Name", &c.ESP->UserNames);
            Check("Weapon Name", &c.ESP->WeaponName);
            Check("Distance", &c.ESP->DistanceFromMe);
            if (Check("Skeleton", &c.ESP->Skeleton) && Core::SDK::Pointers::pLocalPlayer)
                Core::SDK::Pointers::pLocalPlayer->RemoveKinematics();
            ColorRow("Skeleton Color", c.ESP->SkeletonCol);
            Check("Health Bar", &c.ESP->HealthBar);
            Check("Armor Bar", &c.ESP->ArmorBar);
            Check("SnapLines", &c.ESP->SnapLines);
            if (Check("Box", &c.ESP->Box) && !c.ESP->Box) c.ESP->FilledBox = false;
            if (Check("Filled Box", &c.ESP->FilledBox) && c.ESP->FilledBox) c.ESP->Box = true;
            ColorRow("Name Color", c.ESP->UserNamesCol);
            ColorRow("Weapon Color", c.ESP->WeaponNameCol);
            ColorRow("Distance Color", c.ESP->DistanceCol);
        }
        EndPanel();
    }

    inline void Vehicles()
    {
        const ImVec2 size = PanelSize(200);
        auto& c = Core::g_Config;
        if (Panel("VEHICLE", size)) {
            Check("Enabled", &c.VehicleESP->Enabled);
            Slider("Render Distance", &c.VehicleESP->MaxDistance, 0, 1000, "%dm");
        }
        EndPanel();
        ImGui::SameLine(0, Gap);
        if (Panel("VEHICLE SETTINGS", size)) {
            Check("Name", &c.VehicleESP->VehName);
            Check("Distance", &c.VehicleESP->DistanceFromMe);
            Check("Lock Status", &c.VehicleESP->ShowLockUnlock);
            Check("SnapLines", &c.VehicleESP->SnapLines);
            ColorRow("SnapLines Color", c.VehicleESP->SnapLinesCol);
        }
        EndPanel();
    }
}
