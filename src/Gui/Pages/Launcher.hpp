#pragma once
// Launcher: post-login gate between authentication and the main panel.
// Flow: Login -> Launcher (Inject | Descricao | Update) -> Panel.
// The panel only starts when the user presses Inject; the auth system is
// untouched — Inject re-validates via g_Auth.RequireAuth() and refuses to
// start while a mandatory update is pending.
//
// Visual identity reuses the project UI kit (UI::NavItem sidebar with
// animated yellow indicator, UI::Particles background, UI::PrimaryButton,
// UI::BeginCard), same window footprint as the main panel (880x460).
#include <Gui/UI.hpp>
#include <Gui/ReferenceMenu.hpp>
#include <Core/Update/UpdateConfig.hpp>
#include <Core/Update/UpdateManager.hpp>
#include <Auth/auth_manager.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

#include <string>

namespace Launcher
{
    constexpr float Width = 880.f, Height = 460.f;
    constexpr float SidebarWidth = 230.f;

    inline int Tab = 0; // 0 = Inject, 1 = Descricao, 2 = Update
    inline float TabFade = 1.f;
    inline bool Injected = false;
    inline bool InjectBusy = false;
    inline std::string InjectError;
    inline bool CheckStarted = false;
    inline bool VersionSeeded = false;

    inline void Reset()
    {
        Tab = 0;
        TabFade = 1.f;
        Injected = false;
        InjectBusy = false;
        InjectError.clear();
        CheckStarted = false;
    }

    inline bool IsInjected() { return Injected; }

    inline void RenderInject();
    inline void RenderDescription();
    inline bool RenderUpdate();

    inline const char* StatusDisplay(const std::string& status)
    {
        if (status == "Manutencao") return "Manutenção";
        if (status == "Indisponivel") return "Indisponível";
        return "Disponível";
    }

    inline ImVec4 StatusColor(const std::string& status)
    {
        if (status == "Disponivel") return UI::Accent();
        return UI::TextDim();
    }

    // Returns true when the application should exit (updater launched).
    inline bool Render()
    {
        if (!VersionSeeded) {
            VersionSeeded = true;
            g_Variables.version = Update::InstalledDisplay();
        }

        const float menuAlpha = ImGui::GetStyle().Alpha;
        if (menuAlpha <= 0.01f)
            return false;

        // First frame on screen: kick off the update check (async, UI stays
        // responsive; results are polled below every frame).
        if (!CheckStarted) {
            CheckStarted = true;
            Update::Manager().CheckAsync();
        }

        const ImVec2 p = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Backdrop: black canvas, rounded corners, soft border.
        draw->AddRectFilled(p, p + size, ImGui::GetColorU32(UI::Background()), 12.f);
        draw->AddRect(p, p + size, ImGui::GetColorU32(UI::Border()), 12.f);
        // Gold particle field behind everything, clipped to the window.
        UI::Particles::DrawBehind(p + ImVec2(4, 4), p + size - ImVec2(4, 4),
            ImGui::GetIO().DeltaTime);

        // Sidebar separator.
        draw->AddLine(p + ImVec2(SidebarWidth, 24), p + ImVec2(SidebarWidth, size.y - 24),
            ImGui::GetColorU32(UI::BorderSoft()));

        // Brand block.
        ImFont* brand = UI::SafeFont(g_Variables.m_FontSecundary);
        draw->AddText(brand, 26.f, p + ImVec2(24, 22), ImGui::GetColorU32(UI::Text()), "svchost");
        draw->AddText(p + ImVec2(25, 56), ImGui::GetColorU32(UI::Accent()), "LAUNCHER");
        ImFont* verFont = UI::SafeFont(g_Variables.m_FontSmaller);
        const std::string verTxt = Update::InstalledDisplay();
        draw->AddText(verFont, verFont->FontSize, p + ImVec2(25, 76),
            ImGui::GetColorU32(UI::TextDim()), verTxt.c_str());

        // Sidebar navigation (animated yellow indicator comes from NavItem).
        ImGui::SetCursorPos(ImVec2(14, 112));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        ImGui::BeginChild("##launcher_nav", ImVec2(SidebarWidth - 28, 220), false);
        const int prevTab = Tab;
        if (UI::NavItem("##nav_inject", ICON_FA_SYRINGE, "Inject", Tab == 0, SidebarWidth - 28)) Tab = 0;
        if (UI::NavItem("##nav_desc", ICON_FA_CIRCLE_INFO, "Descrição", Tab == 1, SidebarWidth - 28)) Tab = 1;
        if (UI::NavItem("##nav_update", ICON_FA_DOWNLOAD, "Update", Tab == 2, SidebarWidth - 28)) Tab = 2;
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        if (Tab != prevTab) TabFade = 0.f;
        TabFade = ImLerp(TabFade, 1.f, UI::MotionStep(10.f));

        // User chip at the sidebar bottom.
        const ImVec2 chip = p + ImVec2(14, size.y - 52);
        draw->AddRectFilled(chip, chip + ImVec2(SidebarWidth - 28, 38),
            ImGui::GetColorU32(UI::Surface2()), 8.f);
        ImFont* iconF = UI::SafeFont(g_Variables.FontAwesomeSolidSmall);
        draw->AddText(iconF, iconF->FontSize, chip + ImVec2(12, 11),
            ImGui::GetColorU32(UI::Accent()), ICON_FA_USER);
        draw->PushClipRect(chip + ImVec2(36, 0), chip + ImVec2(SidebarWidth - 36, 38), true);
        draw->AddText(chip + ImVec2(36, 4), ImGui::GetColorU32(UI::Text()),
            g_Variables.UserName.c_str());
        draw->AddText(chip + ImVec2(36, 20), ImGui::GetColorU32(UI::TextDim()),
            g_Variables.Role.c_str());
        draw->PopClipRect();

        // Content area with a soft tab transition.
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ImClamp(TabFade, 0.f, 1.f));
        ImGui::SetCursorPos(ImVec2(SidebarWidth + 26, 22));
        ImGui::BeginChild("##launcher_content",
            ImVec2(size.x - SidebarWidth - 40, size.y - 36), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        bool requestClose = false;
        if (Tab == 0) RenderInject();
        else if (Tab == 1) RenderDescription();
        else requestClose = RenderUpdate();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        return requestClose;
    }

    inline void SectionHead(const char* title, const char* subtitle)
    {
        ImFont* t = UI::SafeFont(g_Variables.m_FontSecundary);
        ImGui::PushFont(t);
        ImGui::TextColored(UI::Text(), "%s", title);
        ImGui::PopFont();
        ImGui::TextColored(UI::TextDim(), "%s", subtitle);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, UI::BorderSoft());
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    inline void RenderInject()
    {
        SectionHead("Inject", "Clique no botão para iniciar.");

        const Update::UpdateSnapshot snap = Update::Manager().Poll();
        if (snap.mandatory) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .55f, .55f, 1.f));
            ImGui::TextWrapped("Atualização obrigatória pendente: %s",
                snap.mandatoryReason.empty() ? "atualize para continuar." : snap.mandatoryReason.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        if (!InjectError.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .55f, .55f, 1.f));
            ImGui::TextWrapped("%s", InjectError.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // Centered large yellow button.
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 btnSize(320.f, 64.f);
        ImGui::Dummy(ImVec2(0, 40.f));
        ImGui::SetCursorPosX((avail.x - btnSize.x) * .5f);
        const bool blocked = InjectBusy || snap.mandatory;
        ImGui::BeginDisabled(blocked);
        ImFont* bf = UI::SafeFont(g_Variables.m_FontSecundary);
        ImGui::PushFont(bf);
        const bool pressed = UI::PrimaryButton(
            InjectBusy ? "Iniciando..." : "Inject", btnSize);
        ImGui::PopFont();
        ImGui::EndDisabled();

        if (pressed && !blocked) {
            InjectBusy = true;
            InjectError.clear();
            // Re-validate the session at click time (auth untouched).
            // On failure the login screen shows g_Auth.errorMsg (set by
            // RequireAuth); nothing to display here since we leave IsLogged.
            if (!g_Auth.RequireAuth()) {
                g_MenuInfo.IsLogged = false;
                g_MenuInfo.IsOpen = false;
                Reset();
                InjectBusy = false;
            } else if (Update::Manager().MandatoryPending()) {
                InjectError = "Conclua a atualização obrigatória antes de continuar.";
                Tab = 2;
                InjectBusy = false;
            } else {
                Injected = true;
                g_MenuInfo.IsOpen = true;
                g_MenuInfo.MenuSize = { ReferenceMenu::Width, ReferenceMenu::Height };
                NotifyManager::Send("Painel iniciado.", 2500);
                InjectBusy = false;
            }
        }

        ImGui::Dummy(ImVec2(0, 24.f));
        ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(
            ("Versão " + Update::InstalledDisplay()).c_str()).x) * .5f);
        ImGui::TextColored(UI::TextDim(), "Versão %s", Update::InstalledDisplay().c_str());
    }

    inline void RenderDescription()
    {
        using namespace Update;
        SectionHead("Descrição", "Informações sobre o produto.");

        ImGui::TextColored(UI::Text(), "%s", ChannelConfig::ProductName());
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
        ImGui::TextColored(UI::TextDim(), "%s", ChannelConfig::AppDescription());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        const UpdateSnapshot snap = Manager().Poll();
        const std::string status = snap.status.empty()
            ? ChannelConfig::ServiceStatusDefault() : snap.status;
        const float cardW = (ImGui::GetContentRegionAvail().x - 16.f) / 3.f;

        ImGui::BeginGroup();
        if (UI::BeginCard("##card_ver", nullptr, ImVec2(cardW, 118))) {
            ImGui::TextColored(UI::TextDim(), "Versão");
            ImFont* f = UI::SafeFont(g_Variables.m_FontSecundary);
            ImGui::PushFont(f);
            ImGui::TextColored(UI::Accent(), "%s", InstalledDisplay().c_str());
            ImGui::PopFont();
        }
        UI::EndCard();
        ImGui::EndGroup();
        ImGui::SameLine(0, 8);
        ImGui::BeginGroup();
        if (UI::BeginCard("##card_status", nullptr, ImVec2(cardW, 118))) {
            ImGui::TextColored(UI::TextDim(), "Status");
            ImFont* f = UI::SafeFont(g_Variables.m_FontSecundary);
            ImGui::PushFont(f);
            ImGui::TextColored(StatusColor(status), "%s", StatusDisplay(status));
            ImGui::PopFont();
        }
        UI::EndCard();
        ImGui::EndGroup();
        ImGui::SameLine(0, 8);
        ImGui::BeginGroup();
        if (UI::BeginCard("##card_sup", nullptr, ImVec2(cardW, 118))) {
            ImGui::TextColored(UI::TextDim(), "Suporte");
            ImGui::TextColored(UI::Text(), "Discord");
            if (UI::SecondaryButton("Copiar link", ImVec2(cardW - 32, 30))) {
                ImGui::SetClipboardText(ChannelConfig::DiscordUrl());
                NotifyManager::Send("Link do Discord copiado.", 2000);
            }
        }
        UI::EndCard();
        ImGui::EndGroup();
    }

    // Returns true when the app should exit (updater launched).
    inline bool RenderUpdate()
    {
        using namespace Update;
        SectionHead("Update", "Verifique se há uma versão nova.");

        UpdateManager& mgr = Manager();
        const UpdateSnapshot snap = mgr.Poll();
        const ImVec2 avail = ImGui::GetContentRegionAvail();

        auto centerText = [&](const char* txt, ImVec4 col) {
            ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(txt).x) * .5f);
            ImGui::TextColored(col, "%s", txt);
        };

        switch (snap.state) {
        case UpdateState::Idle:
        case UpdateState::Checking: {
            ImGui::Dummy(ImVec2(0, 30.f));
            ImGui::SetCursorPosX((avail.x - 28.f) * .5f);
            UI::Spinner("##upd_spin", 14.f, 3.f, UI::Accent());
            ImGui::Dummy(ImVec2(0, 12.f));
            centerText("Verificando atualizações...", UI::TextDim());
            break;
        }
        case UpdateState::UpToDate: {
            ImGui::Dummy(ImVec2(0, 24.f));
            ImFont* f = UI::SafeFont(g_Variables.FontAwesomeSolid);
            ImGui::SetCursorPosX((avail.x - 44.f) * .5f);
            ImGui::PushFont(f);
            ImGui::TextColored(UI::Accent(), "%s", ICON_FA_CIRCLE_CHECK);
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(0, 8.f));
            centerText("Tudo atualizado", UI::Text());
            char buf[64];
            sprintf_s(buf, "Você está na versão %s", snap.installed.c_str());
            centerText(buf, UI::TextDim());
            ImGui::Dummy(ImVec2(0, 16.f));
            ImGui::SetCursorPosX((avail.x - 280.f) * .5f);
            ImGui::BeginDisabled(true);
            UI::SecondaryButton("Já está atualizado", ImVec2(280.f, 44.f));
            ImGui::EndDisabled();
            ImGui::Dummy(ImVec2(0, 8.f));
            ImGui::SetCursorPosX((avail.x - 280.f) * .5f);
            if (UI::SecondaryButton("Verificar novamente", ImVec2(280.f, 34.f)))
                mgr.CheckAsync();
            break;
        }
        case UpdateState::Available: {
            centerText("Nova atualização disponível", UI::Accent());
            char buf[128];
            sprintf_s(buf, "Instalada: %s  →  Nova: %s",
                snap.installed.c_str(), snap.available.c_str());
            centerText(buf, UI::TextDim());
            if (!snap.notes.empty()) {
                ImGui::Spacing();
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + avail.x);
                ImGui::TextColored(UI::TextDim(), "%s", snap.notes.c_str());
                ImGui::PopTextWrapPos();
            }
            ImGui::Dummy(ImVec2(0, 12.f));
            ImGui::SetCursorPosX((avail.x - 280.f) * .5f);
            if (UI::PrimaryButton(snap.mandatory ? "Atualização obrigatória: Atualizar agora"
                                                : "Atualizar agora",
                    ImVec2(280.f, 48.f)))
                mgr.DownloadAsync();
            break;
        }
        case UpdateState::Downloading: {
            char buf[128];
            sprintf_s(buf, "Baixando %s... %d%%", snap.available.c_str(),
                (int)(snap.progress * 100.f));
            centerText(buf, UI::Text());
            ImGui::Dummy(ImVec2(0, 8.f));
            ImGui::SetCursorPosX((avail.x - 420.f) * .5f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, UI::Accent());
            ImGui::PushStyleColor(ImGuiCol_FrameBg, UI::Surface2());
            ImGui::ProgressBar(snap.progress, ImVec2(420.f, 22.f), "");
            ImGui::PopStyleColor(2);
            if (snap.totalBytes > 0) {
                char b2[128];
                sprintf_s(b2, "%llu / %llu KB",
                    snap.doneBytes / 1024ULL, snap.totalBytes / 1024ULL);
                centerText(b2, UI::TextDim());
            }
            ImGui::Dummy(ImVec2(0, 8.f));
            ImGui::SetCursorPosX((avail.x - 200.f) * .5f);
            if (UI::SecondaryButton("Cancelar", ImVec2(200.f, 34.f)))
                mgr.Cancel();
            break;
        }
        case UpdateState::Verifying: {
            ImGui::Dummy(ImVec2(0, 30.f));
            ImGui::SetCursorPosX((avail.x - 28.f) * .5f);
            UI::Spinner("##upd_verify", 14.f, 3.f, UI::Accent());
            ImGui::Dummy(ImVec2(0, 12.f));
            centerText("Verificando integridade e assinatura...", UI::TextDim());
            break;
        }
        case UpdateState::Ready: {
            centerText("Pronto para instalar", UI::Accent());
            char buf[128];
            sprintf_s(buf, "Versão %s verificada e pronta.", snap.available.c_str());
            centerText(buf, UI::TextDim());
            ImGui::Dummy(ImVec2(0, 12.f));
            ImGui::SetCursorPosX((avail.x - 280.f) * .5f);
            if (UI::PrimaryButton("Instalar e reiniciar", ImVec2(280.f, 48.f))) {
                if (mgr.InstallAsync())
                    return true; // updater took over: exit now
            }
            break;
        }
        case UpdateState::Installing: {
            centerText("Instalando atualização...", UI::Text());
            centerText("O aplicativo será reiniciado.", UI::TextDim());
            return true;
        }
        case UpdateState::Error: {
            ImGui::Dummy(ImVec2(0, 24.f));
            centerText("Não foi possível atualizar", UI::Text());
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + avail.x);
            ImGui::SetCursorPosX(0);
            ImGui::TextColored(ImVec4(1.f, .55f, .55f, 1.f), "%s",
                snap.error.empty() ? "Erro desconhecido." : snap.error.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Dummy(ImVec2(0, 12.f));
            ImGui::SetCursorPosX((avail.x - 280.f) * .5f);
            if (UI::PrimaryButton("Tentar novamente", ImVec2(280.f, 44.f)))
                mgr.CheckAsync();
            break;
        }
        }
        return false;
    }
} // namespace Launcher
