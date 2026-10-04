#pragma once
// language: C++, file: Gui/Pages/Login.hpp, target: Windows x64, MSVC
// Telas de Login e Register integradas com AuthManager / KeyAuth 1.3.
// Design: preserva paleta BLACK+YELLOW do ASTRA (UI.hpp).

#include <Includes/Includes.hpp>
#include <Gui/UI.hpp>
#include <Gui/ReferenceMenu.hpp>
#include <Auth/auth_manager.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/Core.hpp>

namespace LoginUI
{
    // ─── Buffers da UI (nunca impressos / salvos em disco) ──────────────────
    static char  bufUser[64]    = {};
    static char  bufPass[64]    = {};
    static char  bufConfirm[64] = {};
    static char  bufKey[128]    = {};
    static bool  showPass       = false;
    static bool  showConfirm    = false;

    // ─── Limpa buffers sensíveis ─────────────────────────────────────────────
    static void ClearSensitive()
    {
        SecureZeroMemory(bufPass,    sizeof(bufPass));
        SecureZeroMemory(bufConfirm, sizeof(bufConfirm));
        SecureZeroMemory(bufKey,     sizeof(bufKey));
        showPass    = false;
        showConfirm = false;
    }

    // ─── Helper: campo de input com label e foco amarelo ────────────────────
    static bool InputField(ImDrawList*  draw,
                           const ImVec2& origin,
                           const char*  label,
                           const char*  id,
                           const char*  hint,
                           char*        buf,
                           size_t       bufSz,
                           float        left,
                           float        width,
                           float        y,
                           ImGuiInputTextFlags flags)
    {
        ImGui::SetCursorPos(ImVec2(left, y));
        ImGui::TextColored(UI::Text(), "%s", label);
        ImGui::SetCursorPos(ImVec2(left, y + 24.f));
        ImGui::SetNextItemWidth(width);
        UI::PushInputStyle(false);
        bool edited = ImGui::InputTextWithHint(id, hint, buf, bufSz, flags);

        // Anel amarelo animado no foco
        static std::map<ImGuiID, float> focusAnim;
        ImGuiID wid = ImGui::GetID(id);
        float& strength = focusAnim[wid];
        strength = ImLerp(strength,
            ImGui::IsItemActive() ? 1.f : (ImGui::IsItemHovered() ? .3f : 0.f),
            UI::MotionStep());
        draw->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::GetColorU32(ImVec4(1.f, .83f, 0.f, strength)), 7.f, 0, 1.5f);

        UI::PopInputStyle();
        return edited;
    }

    // ─── Helper: ícone de olho para mostrar/esconder senha ──────────────────
    static bool EyeToggle(const char* id, bool& show, float x, float y)
    {
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, .83f, 0.f, .12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.f, .83f, 0.f, .2f));
        ImGui::PushStyleColor(ImGuiCol_Text,          UI::TextDim());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
        bool pressed = ImGui::Button(show ? "Hide##" : "Show##", ImVec2(48.f, 22.f));
        if (pressed) show = !show;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        return pressed;
    }

    // ─── Tela de Login ───────────────────────────────────────────────────────
    static void RenderLogin(ImDrawList* draw, const ImVec2& origin, float left, float width)
    {
        // Título e subtítulo
        ImFont* brand = UI::SafeFont(g_Variables.m_FontSecundary);

        draw->AddText(brand, 28.f, origin + ImVec2(left, 44.f),
            ImGui::GetColorU32(UI::Text()), "Welcome back");
        draw->AddText(origin + ImVec2(left, 80.f),
            ImGui::GetColorU32(UI::Accent()), "ASTRA");
        draw->AddText(origin + ImVec2(left, 100.f),
            ImGui::GetColorU32(UI::TextDim()), "Sign in to your account to continue.");

        // Campos
        InputField(draw, origin, "Username", "##ka_user",
                   "Your username", bufUser, sizeof(bufUser),
                   left, width, 148.f, ImGuiInputTextFlags_None);

        // Senha com toggle
        ImGuiInputTextFlags passFlags = showPass
            ? ImGuiInputTextFlags_None
            : ImGuiInputTextFlags_Password;
        InputField(draw, origin, "Password", "##ka_pass",
                   "Your password", bufPass, sizeof(bufPass),
                   left, width - 56.f, 222.f, passFlags);
        EyeToggle("##eye_pass", showPass, left + width - 52.f, 248.f);

        // Área de erro
        bool locked = g_Auth.IsLocked();
        bool inProg = g_Auth.requestInProgress.load();

        if (!g_Auth.errorMsg.empty() && !locked)
        {
            float ey = 302.f;
            draw->AddRectFilled(origin + ImVec2(left, ey - 6.f),
                origin + ImVec2(left + width, ey + 30.f),
                ImGui::GetColorU32(ImVec4(.8f, .2f, .2f, .12f)), 6.f);
            ImGui::SetCursorPos(ImVec2(left + 8.f, ey));
            ImGui::TextColored(ImVec4(1.f, .55f, .55f, 1.f),
                "%s", g_Auth.errorMsg.c_str());
        }
        if (locked)
        {
            int rem = g_Auth.LockRemainingSec();
            float ey = 302.f;
            draw->AddRectFilled(origin + ImVec2(left, ey - 6.f),
                origin + ImVec2(left + width, ey + 30.f),
                ImGui::GetColorU32(ImVec4(.9f, .5f, .0f, .12f)), 6.f);
            ImGui::SetCursorPos(ImVec2(left + 8.f, ey));
            char tmp[80];
            snprintf(tmp, sizeof(tmp), "Too many attempts. Try again in %d seconds.", rem);
            ImGui::TextColored(ImVec4(1.f, .75f, .3f, 1.f), "%s", tmp);
        }

        // Botão LOGIN
        bool canSubmit = !locked && !inProg && g_Auth.initOk;
        ImGui::SetCursorPos(ImVec2(left, 348.f));

        if (!canSubmit)
        {
            ImGui::BeginDisabled();
            UI::PrimaryButton(inProg ? "Signing in..." : (locked ? "Locked" : "Connecting..."),
                ImVec2(width, 44.f));
            ImGui::EndDisabled();
        }
        else
        {
            bool submit = UI::PrimaryButton("Login", ImVec2(width, 44.f));
            bool enter  = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                          (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                           ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false));

            if (submit || enter)
            {
                std::string err;
                if (!g_Auth.ValidateLogin(bufUser, bufPass, err))
                {
                    g_Auth.errorMsg = err;
                }
                else
                {
                    g_Auth.errorMsg.clear();
                    g_Auth.LoginAsync(std::string(bufUser),
                                      std::string(bufPass));
                }
            }
        }

        // Spinner animado enquanto carrega
        if (inProg)
        {
            ImGui::SetCursorPos(ImVec2(left + width + 6.f, 358.f));
            UI::Spinner("##spin_login", 8.f, 2.f, UI::Accent());
        }

        // Link para registro
        draw->AddLine(origin + ImVec2(left, 414.f),
            origin + ImVec2(left + width, 414.f),
            ImGui::GetColorU32(UI::BorderSoft()));
        ImGui::SetCursorPos(ImVec2(left, 424.f));
        ImGui::TextColored(UI::TextDim(), "Don't have an account?");
        ImGui::SetCursorPos(ImVec2(left, 444.f));
        if (UI::SecondaryButton("Create Account", ImVec2(width, 38.f)))
        {
            g_Auth.page     = AuthPage::Register;
            g_Auth.errorMsg.clear();
            ClearSensitive();
            SecureZeroMemory(bufUser, sizeof(bufUser));
        }
    }

    // ─── Tela de Register ────────────────────────────────────────────────────
    static void RenderRegister(ImDrawList* draw, const ImVec2& origin, float left, float width)
    {
        ImFont* brand = UI::SafeFont(g_Variables.m_FontSecundary);

        draw->AddText(brand, 28.f, origin + ImVec2(left, 44.f),
            ImGui::GetColorU32(UI::Text()), "Create Account");
        draw->AddText(origin + ImVec2(left, 80.f),
            ImGui::GetColorU32(UI::Accent()), "ASTRA");
        draw->AddText(origin + ImVec2(left, 100.f),
            ImGui::GetColorU32(UI::TextDim()), "A license key is required to register.");

        // Campos
        InputField(draw, origin, "Username", "##reg_user",
                   "Choose a username", bufUser, sizeof(bufUser),
                   left, width, 136.f, ImGuiInputTextFlags_None);

        ImGuiInputTextFlags pf = showPass ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
        InputField(draw, origin, "Password", "##reg_pass",
                   "Choose a password", bufPass, sizeof(bufPass),
                   left, width - 56.f, 196.f, pf);
        EyeToggle("##eye_reg", showPass, left + width - 52.f, 222.f);

        ImGuiInputTextFlags cf = showConfirm ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
        InputField(draw, origin, "Confirm Password", "##reg_confirm",
                   "Re-enter your password", bufConfirm, sizeof(bufConfirm),
                   left, width - 56.f, 254.f, cf);
        EyeToggle("##eye_confirm", showConfirm, left + width - 52.f, 280.f);

        InputField(draw, origin, "License Key", "##reg_key",
                   "Enter your license key", bufKey, sizeof(bufKey),
                   left, width, 312.f, ImGuiInputTextFlags_None);

        // Área de erro
        bool locked = g_Auth.IsLocked();
        bool inProg = g_Auth.requestInProgress.load();

        if (!g_Auth.errorMsg.empty() && !locked)
        {
            float ey = 368.f;
            draw->AddRectFilled(origin + ImVec2(left, ey - 6.f),
                origin + ImVec2(left + width, ey + 30.f),
                ImGui::GetColorU32(ImVec4(.8f, .2f, .2f, .12f)), 6.f);
            ImGui::SetCursorPos(ImVec2(left + 8.f, ey));
            ImGui::TextColored(ImVec4(1.f, .55f, .55f, 1.f),
                "%s", g_Auth.errorMsg.c_str());
        }
        if (locked)
        {
            int rem = g_Auth.LockRemainingSec();
            float ey = 368.f;
            draw->AddRectFilled(origin + ImVec2(left, ey - 6.f),
                origin + ImVec2(left + width, ey + 30.f),
                ImGui::GetColorU32(ImVec4(.9f, .5f, .0f, .12f)), 6.f);
            ImGui::SetCursorPos(ImVec2(left + 8.f, ey));
            char tmp[80];
            snprintf(tmp, sizeof(tmp), "Too many attempts. Try again in %d seconds.", rem);
            ImGui::TextColored(ImVec4(1.f, .75f, .3f, 1.f), "%s", tmp);
        }

        // Botão CREATE ACCOUNT
        bool canSubmit = !locked && !inProg && g_Auth.initOk;
        ImGui::SetCursorPos(ImVec2(left, 408.f));

        if (!canSubmit)
        {
            ImGui::BeginDisabled();
            UI::PrimaryButton(inProg ? "Creating account..." : (locked ? "Locked" : "Connecting..."),
                ImVec2(width, 44.f));
            ImGui::EndDisabled();
        }
        else
        {
            bool submit = UI::PrimaryButton("Create Account", ImVec2(width, 44.f));
            bool enter  = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                          (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                           ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false));

            if (submit || enter)
            {
                std::string err;
                if (!g_Auth.ValidateRegister(bufUser, bufPass, bufConfirm, bufKey, err))
                {
                    g_Auth.errorMsg = err;
                }
                else
                {
                    g_Auth.errorMsg.clear();
                    g_Auth.RegisterAsync(std::string(bufUser),
                                         std::string(bufPass),
                                         std::string(bufKey));
                }
            }
        }

        if (inProg)
        {
            ImGui::SetCursorPos(ImVec2(left + width + 6.f, 420.f));
            UI::Spinner("##spin_reg", 8.f, 2.f, UI::Accent());
        }

        // Link de volta ao login
        draw->AddLine(origin + ImVec2(left, 466.f),
            origin + ImVec2(left + width, 466.f),
            ImGui::GetColorU32(UI::BorderSoft()));
        ImGui::SetCursorPos(ImVec2(left, 474.f));
        ImGui::TextColored(UI::TextDim(), "Already have an account?");
        ImGui::SetCursorPos(ImVec2(left + 0.f, 494.f));
        if (UI::SecondaryButton("Back to Login", ImVec2(width, 32.f)))
        {
            g_Auth.page     = AuthPage::Login;
            g_Auth.errorMsg.clear();
            ClearSensitive();
            SecureZeroMemory(bufUser, sizeof(bufUser));
        }
    }

    // ─── Entry point chamado por Gui.cpp ─────────────────────────────────────
    static void Render(ImDrawList* draw, const ImVec2& origin, float left, float width)
    {
        // Consome resultado assíncrono (safe: chamado apenas na render thread)
        if (g_Auth.async.ready.load() && !g_Auth.requestInProgress.load())
        {
            g_Auth.ConsumeResult();

            // Se autenticado, dispara notificação e atualiza globals
            if (g_Auth.IsAuthenticated())
            {
                g_Variables.UserName = g_Auth.loggedUser;
                g_Variables.Role     = g_Auth.loggedRole;
                g_Variables.g_bPassedByThisVerify = true;
                g_Variables.g_VerifyLogin         = 348975682703ULL;
                g_MenuInfo.IsLogged = true;
                g_MenuInfo.IsOpen = true;
                g_MenuInfo.MenuSize = { ReferenceMenu::Width, ReferenceMenu::Height };
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
            RenderLogin(draw, origin, left, width);
        else if (g_Auth.page == AuthPage::Register)
            RenderRegister(draw, origin, left, width);
    }
}
