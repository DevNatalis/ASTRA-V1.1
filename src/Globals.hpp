#pragma once
#include <Includes/Includes.hpp>

#include <d3d11.h>
#include <D3DX11tex.h>
#include <D3dx9math.h>
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "D3DX11.lib" )

class CVehicle;

class cColors {
public:
	// ASTRA 2.0: BLACK + YELLOW. Base drives all legacy Custom widgets.
	ImVec4 Base = ImColor(255, 212, 0); // accent yellow #FFD400
	ImVec4 PrimaryText = ImColor(245, 245, 245);      // #F5F5F5
	ImVec4 SecundaryText = ImColor(138, 138, 138);    // #8A8A8A
	ImVec4 notify = ImColor(255, 212, 0);

	ImVec4 FeaturesText = ImColor(245, 245, 245);
	ImVec4 SecundaryFeaturesText = ImColor(138, 138, 138);

	ImVec4 BorderCol = ImColor(36, 36, 36);           // #242424
	ImVec4 LinesCol = ImColor(36, 36, 36);
	ImVec4 BackgroundCol = ImColor(7, 7, 7);         // #070707

	ImVec4 ChildCol = ImColor(14, 14, 14);            // #0E0E0E
	ImVec4 ChildBorderCol = ImColor(36, 36, 36);

	ImVec4 TitleBar = ImColor(7, 7, 7);
	ImVec4 TitleBarBorder = ImColor(36, 36, 36);

	ImVec4 SideBar = ImColor(13, 13, 13);             // #0D0D0D
	ImVec4 SideBarBorder = ImColor(36, 36, 36);

	ImVec4 ButtonHovered = ImColor(255, 224, 51);     // #FFE033


	ImVec4 InputBackground = ImColor(14, 14, 14, 255);
	ImVec4 InputBorder = ImColor(36, 36, 36);


	ImVec4 border = ImColor(36, 36, 36, 255);

};

inline cColors g_Col;

class c_globals {
public:
	std::string id;
	std::string version;

	bool g_bPassedByThisVerify;
	uintptr_t g_VerifyLogin;
	bool done;
	bool IsOpen;


	char userLogin[64] = {};
	char passLogin[64] = {};

	// --- Login screen UI state (visual only, no auth logic change) ---
	bool showPassword = false;
	bool loginLoading = false;
	float loginLoadingT = 0.f;
	char loginError[128] = {};

	DWORD ProcIdFiveM = 0;

	std::string ServerIp;
	std::string UserName;
	std::string Role;

	ImVec2 TestePos;

	char m_Config[6000];
	HWND g_hCheatWindow;
	HWND g_hGameWindow;
	ImVec2 g_vGameWindowSize;
	ImVec2 g_vGameWindowPos;
	ImVec2 g_vGameWindowCenter;

	ImFont* m_FontBig;
	ImFont* m_FontBigSmall;
	ImFont* m_FontNormal;
	ImFont* m_FontSecundary;
	ImFont* m_FontSmaller;
	ImFont* m_DrawFont;
	ImFont* m_Expand;

	ImFont* FontAwesomeSolid;
	ImFont* FontAwesomeSolidSmall;
	ImFont* FontAwesomeRegular;
	ImFont* FontAwesomeBrands;

	ImFont* lexend_font = nullptr;
	ImFont* lexend_fontbig = nullptr;
	ImFont* lexend_fontsmall = nullptr;
	ImFont* font_awesome = nullptr;

	ID3D11ShaderResourceView* Logo = nullptr;
	ID3D11ShaderResourceView* decor_background = nullptr;

	struct FiveM_Weapons_t {
		ID3D11ShaderResourceView* gadget_parachute = nullptr;
		ID3D11ShaderResourceView* weapon_advancedrifle = nullptr;
		ID3D11ShaderResourceView* weapon_appistol = nullptr;
		ID3D11ShaderResourceView* weapon_assaultrifle = nullptr;
		ID3D11ShaderResourceView* weapon_assaultrifle_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_assaultshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_assaultsmg = nullptr;
		ID3D11ShaderResourceView* weapon_autoshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_ball = nullptr;
		ID3D11ShaderResourceView* weapon_bat = nullptr;
		ID3D11ShaderResourceView* weapon_battleaxe = nullptr;
		ID3D11ShaderResourceView* weapon_bottle = nullptr;
		ID3D11ShaderResourceView* weapon_bullpuprifle = nullptr;
		ID3D11ShaderResourceView* weapon_bullpuprifle_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_bullpupshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_bzgas = nullptr;
		ID3D11ShaderResourceView* weapon_carbinerifle = nullptr;
		ID3D11ShaderResourceView* weapon_carbinerifle_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_ceramicpistol = nullptr;
		ID3D11ShaderResourceView* weapon_combatmg = nullptr;
		ID3D11ShaderResourceView* weapon_combatmg_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_combatpdw = nullptr;
		ID3D11ShaderResourceView* weapon_combatpistol = nullptr;
		ID3D11ShaderResourceView* weapon_combatshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_compactlauncher = nullptr;
		ID3D11ShaderResourceView* weapon_compactrifle = nullptr;
		ID3D11ShaderResourceView* weapon_crowbar = nullptr;
		ID3D11ShaderResourceView* weapon_dagger = nullptr;
		ID3D11ShaderResourceView* weapon_dbshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_doubleaction = nullptr;
		ID3D11ShaderResourceView* weapon_fireextinguisher = nullptr;
		ID3D11ShaderResourceView* weapon_firework = nullptr;
		ID3D11ShaderResourceView* weapon_flare = nullptr;
		ID3D11ShaderResourceView* weapon_flaregun = nullptr;
		ID3D11ShaderResourceView* weapon_flashlight = nullptr;
		ID3D11ShaderResourceView* weapon_gadgetpistol = nullptr;
		ID3D11ShaderResourceView* weapon_golfclub = nullptr;
		ID3D11ShaderResourceView* weapon_grenade = nullptr;
		ID3D11ShaderResourceView* weapon_grenadelauncher = nullptr;
		ID3D11ShaderResourceView* weapon_grenadelauncher_smoke = nullptr;
		ID3D11ShaderResourceView* weapon_gusenberg = nullptr;
		ID3D11ShaderResourceView* weapon_hammer = nullptr;
		ID3D11ShaderResourceView* weapon_hatchet = nullptr;
		ID3D11ShaderResourceView* weapon_hazardcan = nullptr;
		ID3D11ShaderResourceView* weapon_heavypistol = nullptr;
		ID3D11ShaderResourceView* weapon_heavyshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_heavysniper = nullptr;
		ID3D11ShaderResourceView* weapon_heavysniper_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_hominglauncher = nullptr;
		ID3D11ShaderResourceView* weapon_knife = nullptr;
		ID3D11ShaderResourceView* weapon_knuckle = nullptr;
		ID3D11ShaderResourceView* weapon_machete = nullptr;
		ID3D11ShaderResourceView* weapon_machinepistol = nullptr;
		ID3D11ShaderResourceView* weapon_marksmanpistol = nullptr;
		ID3D11ShaderResourceView* weapon_marksmanrifle = nullptr;
		ID3D11ShaderResourceView* weapon_marksmanrifle_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_mg = nullptr;
		ID3D11ShaderResourceView* weapon_microsmg = nullptr;
		ID3D11ShaderResourceView* weapon_militaryrifle = nullptr;
		ID3D11ShaderResourceView* weapon_minigun = nullptr;
		ID3D11ShaderResourceView* weapon_minismg = nullptr;
		ID3D11ShaderResourceView* weapon_molotov = nullptr;
		ID3D11ShaderResourceView* weapon_musket = nullptr;
		ID3D11ShaderResourceView* weapon_navyrevolver = nullptr;
		ID3D11ShaderResourceView* weapon_nightstick = nullptr;
		ID3D11ShaderResourceView* weapon_petrolcan = nullptr;
		ID3D11ShaderResourceView* weapon_pipebomb = nullptr;
		ID3D11ShaderResourceView* weapon_pistol = nullptr;
		ID3D11ShaderResourceView* weapon_pistol50 = nullptr;
		ID3D11ShaderResourceView* weapon_pistol_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_poolcue = nullptr;
		ID3D11ShaderResourceView* weapon_proxmine = nullptr;
		ID3D11ShaderResourceView* weapon_pumpshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_pumpshotgun_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_railgun = nullptr;
		ID3D11ShaderResourceView* weapon_raycarbine = nullptr;
		ID3D11ShaderResourceView* weapon_rayminigun = nullptr;
		ID3D11ShaderResourceView* weapon_raypistol = nullptr;
		ID3D11ShaderResourceView* weapon_revolver = nullptr;
		ID3D11ShaderResourceView* weapon_revolver_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_rpg = nullptr;
		ID3D11ShaderResourceView* weapon_sawnoffshotgun = nullptr;
		ID3D11ShaderResourceView* weapon_smg = nullptr;
		ID3D11ShaderResourceView* weapon_smg_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_smokegrenade = nullptr;
		ID3D11ShaderResourceView* weapon_sniperrifle = nullptr;
		ID3D11ShaderResourceView* weapon_snowball = nullptr;
		ID3D11ShaderResourceView* weapon_snspistol = nullptr;
		ID3D11ShaderResourceView* weapon_snspistol_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_specialcarbine = nullptr;
		ID3D11ShaderResourceView* weapon_specialcarbine_mk2 = nullptr;
		ID3D11ShaderResourceView* weapon_stickybomb = nullptr;
		ID3D11ShaderResourceView* weapon_stone_hatchet = nullptr;
		ID3D11ShaderResourceView* weapon_stungun = nullptr;
		ID3D11ShaderResourceView* weapon_switchblade = nullptr;
		ID3D11ShaderResourceView* weapon_unarmed = nullptr;
		ID3D11ShaderResourceView* weapon_vintagepistol = nullptr;
		ID3D11ShaderResourceView* weapon_wrench = nullptr;

	}FiveM_WeaponsPic;

};

inline c_globals g_Variables;

class c_menu {
public:

	enum PAGES {
		Combat,
		Visuals,
		VisualsVehicles,
		Vehicless,
		Local,
		World,
		Weapons,
		Settings,
		Login
	};

	bool IsOpen = false;
	int iTabCount = 0;
	float TabAlpha = 0.f;
	int iCurrentPage = 6;
	float TabAdd = 0.f;
	bool IsLogged = false;


	bool particles = true;
	int particleType = 0; 
	bool particles2 = false;



	float backgroundAlpha = .97f;
	int particleCount = 50;
	float minSpeed = 1.0f;
	float maxSpeed = 25.0f;
	float minRadius = 1.0f;
	float maxRadius = 4.0f;

	bool enableRGBParticles = false;
	bool enableRGBRocket = false;
	bool enableRGBInfos = false;
	bool enableRGBSidebarIcons = false;
	bool enableRGBSidebarSelectedIcon = false;


	float rgbSpeed = 0.1f;
	ImColor particlesColor = ImColor(255, 212, 0, 255 / 2);
	ImColor rocketColor = ImColor(255, 255, 255);
	ImColor sidebarIconsColor = ImColor(255, 255, 255);
	ImColor sidebarSelectedIconColor = ImColor(255, 255, 255);

	ImColor originalParticlesColor = ImColor(255, 212, 0, 255 / 2);
	ImColor originalRocketColor = ImColor(255, 255, 255);
	ImColor originalInfosColor = ImColor(255, 255, 255);
	ImColor originalSidebarIconsColor = ImColor(255, 255, 255);
	ImColor originalSidebarSelectedIconColor = ImColor(255, 255, 255);

	ImColor infosColor = ImColor(255, 255, 255);
	char cDiscordId[200];

	ImVec2 MenuSize{ 1050, 600 };

	// Secondary-sidebar navigation state: one subpage index per primary tab.
	// -1 = show all sections of the category.
	int SubPage[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	char Search[64] = "";

	bool isSpectating = false;
	D3DXVECTOR3 spectateOldPos{ 0, 0, 0 };
	CVehicle* spectateTarget = nullptr;
};

inline c_menu g_MenuInfo;