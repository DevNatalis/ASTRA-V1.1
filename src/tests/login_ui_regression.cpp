#include <Gui/Pages/Login.hpp>
#include <stdexcept>

static void Check(bool value, const char* name)
{
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS: " << name << '\n';
}

static bool ButtonFrame(bool down, bool disabled = false)
{
    auto& io = ImGui::GetIO();
    io.MousePos = ImVec2(54, 54);
    io.MouseDown[0] = down;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(300, 180));
    ImGui::Begin("button-test", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::BeginDisabled(disabled);
    const bool pressed = LoginUI::AnimatedButton("X##close_login", ImVec2(28, 28));
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::Render();
    return pressed;
}

static float FormFrame(bool registration, ImVec2 size, bool scrollBottom = false)
{
    ImGui::GetIO().MousePos = ImVec2(-100, -100);
    ImGui::GetIO().MouseDown[0] = false;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("form-preview", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::BeginChild("body", ImVec2(0, 0), false);
    LoginUI::RenderForm(registration, ImGui::GetCursorPosX(), ImGui::GetContentRegionAvail().x);
    if (scrollBottom) ImGui::SetScrollY(ImGui::GetScrollMaxY());
    const float scroll = ImGui::GetScrollMaxY();
    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
    return scroll;
}

static void SavePreview(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* path)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 640; desc.Height = 720;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* target = nullptr;
    Check(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &texture)), "preview render target");
    Check(SUCCEEDED(device->CreateRenderTargetView(texture, nullptr, &target)), "preview target view");
    context->OMSetRenderTargets(1, &target, nullptr);
    const float clear[4] = { .03f, .03f, .03f, 1.f };
    context->ClearRenderTargetView(target, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    Check(SUCCEEDED(D3DX11SaveTextureToFileW(context, texture, D3DX11_IFF_PNG, path)), "saved actual ImGui preview");
    target->Release(); texture->Release();
}

int main()
{
    try {
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(640, 720);
        io.DeltaTime = 1.f / 60.f;
        ImFont* font = io.Fonts->AddFontDefault();
        g_Variables.m_FontNormal = g_Variables.m_FontSecundary = g_Variables.m_FontSmaller = font;
        g_Variables.FontAwesomeSolid = g_Variables.FontAwesomeSolidSmall = font;
        unsigned char* pixels; int w, h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        ImGui::GetStyle().WindowPadding = ImVec2(12, 12);
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = UI::Background();
        ImGui::GetStyle().Colors[ImGuiCol_ChildBg] = UI::Surface();
        ButtonFrame(false); ButtonFrame(false);
        Check(!ButtonFrame(true), "close button waits for release");
        Check(ButtonFrame(false), "close button responds to mouse click");
        ButtonFrame(true, true);
        Check(!ButtonFrame(false, true), "disabled button ignores clicks");
        g_Auth.requestInProgress = true;
        ButtonFrame(true);
        Check(ButtonFrame(false), "close button remains usable during authentication");
        g_Auth.requestInProgress = false;
        g_Auth.initOk = true;
        g_Auth.errorMsg = "Invalid Session. This example checks wrapping of a longer authentication error without overlapping the buttons below.";
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        D3D_FEATURE_LEVEL feature;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &device, &feature, &context)), "headless D3D renderer");
        ImGui_ImplDX11_Init(device, context);
        ImGui_ImplDX11_NewFrame();
        FormFrame(true, ImVec2(442, 660));
        Check(FormFrame(true, ImVec2(442, 660)) == 0.f, "registration fits tall viewport with wrapped error");
        SavePreview(device, context, L"x64/Tests/register-preview.png");
        FormFrame(false, ImVec2(442, 500));
        Check(FormFrame(false, ImVec2(442, 500)) == 0.f, "login fits viewport with wrapped error");
        SavePreview(device, context, L"x64/Tests/login-preview.png");
        FormFrame(true, ImVec2(280, 300));
        Check(FormFrame(true, ImVec2(280, 300)) > 0.f, "small viewport enables vertical scrolling");
        for (int i = 0; i < 120; ++i) FormFrame(true, ImVec2(280, 300), true);
        SavePreview(device, context, L"x64/Tests/narrow-preview.png");
        ImGui_ImplDX11_Shutdown();
        context->Release(); device->Release();
        ImGui::DestroyContext();
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
