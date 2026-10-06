#pragma once
#include <Includes/Includes.hpp>
#include <Gui/LoginWidgets.hpp>
#include <Core/Core.hpp>
#include <Gui/ReferenceMenu.hpp>
#include <Auth/auth_manager.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

namespace LoginUI
{
    static char bufUser[65] = {};
    static char bufPass[257] = {};
    static char bufConfirm[257] = {};
    static char bufKey[129] = {};
    static bool showPass = false;
    static bool showConfirm = false;

    static void ClearSensitive()
    {
        SecureZeroMemory(bufPass, sizeof(bufPass));
        SecureZeroMemory(bufConfirm, sizeof(bufConfirm));
        SecureZeroMemory(bufKey, sizeof(bufKey));
        showPass = showConfirm = false;
    }

    static void Field(const char* label, const char* id, const char* hint,
                      char* buffer, size_t capacity, float left, float width, bool* visible = nullptr)
    {
        ImGui::PushID(id);
        ImGui::SetCursorPosX(left);
        ImGui::TextColored(UI::Text(), "%s", label);
        const float y = ImGui::GetCursorPosY() + 4.f;
        ImGui::SetCursorPos(ImVec2(left, y));
        const float inputWidth = visible ? width - 60.f : width;
        ImGui::SetNextItemWidth(inputWidth);
        UI::PushInputStyle(false);
        ImGui::InputTextWithHint("##input", hint, buffer, capacity,
            visible && !*visible ? ImGuiInputTextFlags_Password : ImGuiInputTextFlags_None);
        const ImVec2 bottom = ImGui::GetItemRectMax();
        static std::map<ImGuiID, float> focus;
        float& amount = focus[ImGui::GetID("##input")];
        amount = ImLerp(amount, ImGui::IsItemActive() ? 1.f : (ImGui::IsItemHovered() ? .35f : 0.f), UI::MotionStep());
        ImVec4 ring = UI::Accent(); ring.w = amount;
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), bottom, ImGui::GetColorU32(ring), 7.f);
        UI::PopInputStyle();
        const float nextY = ImGui::GetCursorPosY() + 10.f;
        if (visible) {
            ImGui::SetCursorPos(ImVec2(left + inputWidth + 8.f, y));
            if (AnimatedButton(*visible ? "Hide###visibility" : "Show###visibility", ImVec2(52.f, bottom.y - ImGui::GetCursorScreenPos().y)))
                *visible = !*visible;
        }
        ImGui::SetCursorPos(ImVec2(left, nextY));
        ImGui::PopID();
    }

    static void ErrorBox(float left, float width)
    {
        std::string message = g_Auth.errorMsg;
        if (g_Auth.IsLocked())
            message = "Too many attempts. Try again in " + std::to_string(g_Auth.LockRemainingSec()) + " seconds.";
        if (message.empty()) return;
        ImGui::SetCursorPosX(left);
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = ImGui::CalcTextSize(message.c_str(), nullptr, false, width - 16.f).y + 16.f;
        ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(width, height),
            ImGui::GetColorU32(ImVec4(.8f, .2f, .2f, .12f)), 6.f);
        ImGui::SetCursorScreenPos(pos + ImVec2(8.f, 8.f));
        ImGui::PushTextWrapPos(left + width - 8.f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .55f, .55f, 1.f));
        ImGui::TextUnformatted(message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::SetCursorScreenPos(pos + ImVec2(0.f, height + 12.f));
    }

    static void RenderForm(bool registration, float left, float width)
    {
        const bool busy = g_Auth.requestInProgress.load() || g_Auth.initializing;
        ImGui::BeginDisabled(busy);
        Field("Username", "user", "Your username", bufUser, sizeof(bufUser), left, width);
        Field("Password", "password", "Your password", bufPass, sizeof(bufPass), left, width, &showPass);
        if (registration) {
            Field("Confirm Password", "confirm", "Re-enter your password", bufConfirm, sizeof(bufConfirm), left, width, &showConfirm);
            Field("License Key", "license", "Enter your license key", bufKey, sizeof(bufKey), left, width);
        }
        ImGui::EndDisabled();
        ErrorBox(left, width);

        if (!g_Auth.initOk && !busy) {
            ImGui::SetCursorPosX(left);
            if (AnimatedButton("Retry connection", ImVec2(width, 40.f))) g_Auth.InitializeAsync();
        } else {
            const bool locked = g_Auth.IsLocked();
            const bool canSubmit = !busy && !locked && g_Auth.initOk;
            const char* label = g_Auth.initializing ? "Connecting..." :
                (busy ? (registration ? "Creating account..." : "Signing in...") :
                (locked ? "Locked" : (registration ? "Create Account" : "Login")));
            ImGui::SetCursorPosX(left);
            ImGui::BeginDisabled(!canSubmit);
            const bool pressed = AnimatedButton(label, ImVec2(width, 44.f));
            ImGui::EndDisabled();
            const bool enter = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false));
            if (canSubmit && (pressed || enter)) {
                std::string error;
                const bool valid = registration ? g_Auth.ValidateRegister(bufUser, bufPass, bufConfirm, bufKey, error) :
                    g_Auth.ValidateLogin(bufUser, bufPass, error);
                if (!valid) g_Auth.errorMsg = error;
                else {
                    g_Auth.errorMsg.clear();
                    if (registration) g_Auth.RegisterAsync(bufUser, bufPass, bufKey);
                    else g_Auth.LoginAsync(bufUser, bufPass);
                }
            }
        }
        ImGui::Dummy(ImVec2(0, 12.f));
        ImGui::SetCursorPosX(left);
        const ImVec2 line = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(line, line + ImVec2(width, 0), ImGui::GetColorU32(UI::BorderSoft()));
        ImGui::Dummy(ImVec2(0, 8.f));
        ImGui::SetCursorPosX(left);
        ImGui::TextColored(UI::TextDim(), "%s", registration ? "Already have an account?" : "Don't have an account?");
        ImGui::SetCursorPosX(left);
        ImGui::BeginDisabled(g_Auth.requestInProgress.load() || g_Auth.initializing);
        if (AnimatedButton(registration ? "Back to Login" : "Create Account", ImVec2(width, 36.f))) {
            g_Auth.page = registration ? AuthPage::Login : AuthPage::Register;
            g_Auth.errorMsg.clear();
            ClearSensitive();
            ImGui::SetScrollY(0);
        }
        ImGui::EndDisabled();
        ImGui::Dummy(ImVec2(0, 12.f));
        ImGui::SetCursorPosX(left);
        ImGui::Checkbox("Menos animacoes", &UI::ReduceMotion);
        ImGui::SetCursorPosX(left);
        ImGui::TextColored(UI::TextDim(), "ASTRA %s", g_Variables.version.empty() ? "v1.0" : g_Variables.version.c_str());
        ImGui::Dummy(ImVec2(0, 8.f));
    }

    static void Render(ImDrawList* draw, const ImVec2& origin, float left, float width)
    {
        // Revalidacao continua: sessao revogada/expirada derruba o menu
        // mesmo que alguem tenha flipado IsLogged em memoria.
        if (g_MenuInfo.IsLogged && !g_Auth.RequireAuth())
        {
            g_MenuInfo.IsLogged = false;
            g_MenuInfo.IsOpen = false;
            if (g_Auth.errorMsg.empty())
                g_Auth.errorMsg = AuthManager::FriendlyMessage(AuthResult::SessionRevoked);
            ClearSensitive();
            SecureZeroMemory(bufUser, sizeof(bufUser));
        }

        // Consome resultado assíncrono (safe: chamado apenas na render thread)
        if (g_Auth.async.ready.load() && !g_Auth.requestInProgress.load())
        {
            g_Auth.ConsumeResult();

            // Se autenticado, dispara notificação e atualiza globals
            if (g_Auth.IsAuthenticated() && g_Auth.RequireAuth())
            {
                g_Variables.UserName = g_Auth.loggedUser;
                g_Variables.Role     = g_Auth.loggedRole;
                g_MenuInfo.IsLogged = true;
                g_MenuInfo.IsOpen = true;
                g_MenuInfo.MenuSize = { ReferenceMenu::Width, ReferenceMenu::Height };
                if (g_Auth.RequireAuth()) // ultima barreira antes de liberar offsets
                    Core::GetOffsets();
                std::string msg = "Login successful! Build: " +
                                  std::to_string(g_Offsets.CurrentBuild);
                NotifyManager::Send(msg.c_str(), 3000);
                ClearSensitive();
                SecureZeroMemory(bufUser, sizeof(bufUser));
                return;
            }
        }

        // Renderiza a página atual
        if (g_Auth.page == AuthPage::Login)
            RenderForm(false, left, width);
        else if (g_Auth.page == AuthPage::Register)
            RenderForm(true, left, width);
    }
}
