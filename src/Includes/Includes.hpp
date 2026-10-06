#pragma once

#define CURL_STATIC_LIB
#define IMGUI_DEFINE_MATH_OPERATORS

#define M_PI 3.14159265358979323846

#pragma warning(push, 0)
#include <Includes/ImGui/Files/imgui.h>
#include <Includes/ImGui/Files/imgui_internal.h>
#include <Includes/ImGui/Files/imgui_impl_win32.h>
#include <Includes/ImGui/Files/imgui_impl_dx11.h>

#include <Includes/ImGui/FontAwesome/FontAwesome.hpp>
#include <Includes/ImGui/FontAwesome/FontAwesomeBrands.hpp>
#include <Includes/ImGui/FontAwesome/RawAwesome6.hpp>

#include <Security/xorstr.hpp>
#include <Security/Api/json.hpp>

#include <Security/Api/curl/curl.h>
#pragma warning(pop)

#include <Globals.hpp>

#include <Core/Config.hpp>

#include <Windows.h>
#include <TlHelp32.h>

#pragma warning(push, 0)
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

#include <d3dx11.h>
#include <d3d11.h>
#include <D3DX11tex.h>
#include <D3dx9math.h>
#pragma warning(pop)

#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "Normaliz.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "Wldap32.lib" )
#pragma comment( lib, "libcurl.lib" )

#pragma comment( lib, "d3d11.lib" )
#ifndef HAS_DXSDK_D3DX
#pragma comment( lib, "D3DX11.lib" )
#endif

using namespace Core;
