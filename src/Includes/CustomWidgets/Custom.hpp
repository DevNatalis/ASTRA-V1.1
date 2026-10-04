#pragma once
#include <Includes/includes.hpp>
#include <Includes/Utils.hpp>
#include <map>


namespace Custom {

	using namespace ImGui;


	namespace c
	{
		const ImVec4 texta = ImVec4(1.f, 1.f, 1.f, 1.f);             // branco
		const ImVec4 textu = ImVec4(60.f / 255.f, 60.f / 255.f, 60.f / 255.f, 1.f); // cinza escuro
		const ImVec4 BGGROUND = ImVec4(29.f / 255.f, 29.f / 255.f, 29.f / 255.f, 1.f); // cinza escuro

		inline ImVec4 accent = ImColor(118, 187, 117);
		inline ImVec4 separator = ImColor(22, 23, 26);

		namespace bg
		{
			inline ImVec2 size = ImVec2(650, 570);
			inline float rounding = 8.f;
		}

	namespace child
	{
		inline ImVec4 background = ImColor(17, 17, 17, 180);
		inline ImVec4 cap = ImColor(20, 21, 23, 200);
		inline float rounding = 14.f;
	}

		namespace page
		{
			inline ImVec4 background_active = ImColor(31, 33, 38, 200);
			inline ImVec4 background = ImColor(22, 23, 25, 180);

			inline ImVec4 text_hov = ImColor(138, 138, 138);
			inline ImVec4 text = ImColor(68, 71, 85);

			inline float rounding = 4.f;
		}

		namespace elements
		{
			inline ImVec4 background_hovered = ImColor(31, 33, 38, 200);
			inline ImVec4 background = ImColor(22, 23, 25, 180);
			inline float rounding = 2.f;
		}

		namespace checkbox
		{
			inline ImVec4 mark = ImColor(0, 0, 0, 255);
		}

		namespace text
		{
			inline ImVec4 text_active = ImColor(255, 255, 255);
			inline ImVec4 text_hov = ImColor(138, 138, 138);
			inline ImVec4 text = ImColor(68, 71, 85);
		}
	}

	inline double EaseInOutCirc(double t)
	{
		if (t < 0.5)
			return (1 - std::sqrt(1 - 2 * t)) * 0.5;
		else
			return (1 + std::sqrt(2 * t - 1)) * 0.5;
	}

	inline int rotation_start_index;
	inline void ImRotateStart()
	{
		rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
	}

	inline ImVec2 ImRotationCenter()
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
	}

	inline void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
	{
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}

	inline void ProfileBar() {
		ImVec2 WindowPos = ImGui::GetWindowPos();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		std::string Username = g_Variables.UserName;
		std::string Role = g_Variables.Role;
		std::string PrimeiraLetra = Username.substr(0, 1);
		std::transform(PrimeiraLetra.begin(), PrimeiraLetra.end(), PrimeiraLetra.begin(), [](unsigned char c) { return std::toupper(c); });

		if (Username.length() > 10) {
			Username = Username.substr(0, 7) + xorstr("...");
		}

		ImVec2 PrimeiraLetraNameTextSize = Utils::CalcTextSize(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, PrimeiraLetra.c_str());
		ImVec2 NameTextSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, Username.c_str());
		ImVec2 RoleTextSize = Utils::CalcTextSize(g_Variables.m_FontSmaller, g_Variables.m_FontSmaller->FontSize, Role.c_str());

		//ImVec2 ProfilePos = { WindowPos.x + g_MenuInfo.MenuSize.x - 30, WindowPos.y + 30 };
		//DrawList->AddCircleFilled( ProfilePos, 20, ImGui::GetColorU32( ( ImVec4 ) ImColor( 17, 17, 20 ) ), 100 );
		//DrawList->AddCircle( ProfilePos, 20, ImGui::GetColorU32( ( ImVec4 ) ( g_Col.BorderCol ) ), 100 );


		//ImVec2 ProfileLetra = { ProfilePos.x - PrimeiraLetraNameTextSize.x / 2, ProfilePos.y - PrimeiraLetraNameTextSize.y / 2 - 1 };
		//DrawList->AddText( g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ProfileLetra, ImGui::GetColorU32( ( ImVec4 ) ImColor( 163, 163, 163 ) ), PrimeiraLetra.c_str( ) );

		//ImVec2 UsernamePos = { ProfilePos.x - 32 - NameTextSize.x, ProfileLetra.y - 6 };
		//DrawList->AddText( g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, UsernamePos, ImGui::GetColorU32( ( ImVec4 ) ImColor( 163, 163, 163 ) ), Username.c_str( ) );

		//ImVec2 RolePos = { ProfilePos.x - 32 - RoleTextSize.x, ProfileLetra.y + 8 };
		//DrawList->AddText( g_Variables.m_FontSmaller, g_Variables.m_FontSmaller->FontSize, RolePos, ImGui::GetColorU32( ( ImVec4 ) ImColor( 80, 80, 80 ) ), Role.c_str( ) );
	}

	inline void DrawBackground(bool Logged) {
		// ASTRA 2.0: dark glass. Alpha lives ONLY on surfaces, never on text.
		const ImVec2 pos = ImGui::GetWindowPos();

		const float round = 10.0f;
		ImU32 bg = ImGui::GetColorU32(ImVec4(0x07 / 255.f, 0x07 / 255.f, 0x07 / 255.f, 0.98f));
		ImU32 sidebar = ImGui::GetColorU32(ImVec4(0x0D / 255.f, 0x0D / 255.f, 0x0D / 255.f, 1.f));
		ImU32 border = ImGui::GetColorU32(ImVec4(0x24 / 255.f, 0x24 / 255.f, 0x24 / 255.f, 1.f));

		if (Logged)
		{
			const float SW = 210.0f;  // single compact sidebar
			ImVec2 end(pos.x + g_MenuInfo.MenuSize.x, pos.y + g_MenuInfo.MenuSize.y);
			ImDrawList* draw = ImGui::GetWindowDrawList();
			draw->AddRectFilled(pos, end, bg, round);
			draw->AddRectFilled(pos, ImVec2(pos.x + SW, end.y), sidebar, round, ImDrawFlags_RoundCornersLeft);
			draw->AddLine(ImVec2(pos.x + SW, pos.y + 8), ImVec2(pos.x + SW, end.y - 8), border, 1.f);
			draw->AddRect(pos, end, border, round, 0, 1.0f);
		}
		else
		{
			// Login: janela totalmente transparente — só o card é desenhado.
			(void)bg; (void)sidebar; (void)border; (void)round;
		}
	}

	struct tab_state
	{
		float arrow_opticaly;
		float text_offset;
		ImVec4 text;
		ImVec4 background, rect;
		float leftLineAlpha = 0.f;  // alpha da linha esquerda
	};

	inline ImVec4 accent_color = ImColor(100, 100, 100, 255);

	inline bool TabBeta(bool selectable, const char* icon, const char* label, const ImVec2& size_arg)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImVec2 icon_size = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, icon);
		const ImVec2 pos = window->DC.CursorPos;

		ImVec2 size = CalcItemSize(size_arg, label_size.x + icon_size.x + 8.0f, ImMax(label_size.y, icon_size.y));
		const ImRect bb(pos, pos + size);

		ImGui::ItemSize(size, 0.f);
		if (!ImGui::ItemAdd(bb, id)) return false;

		bool hovered, held, pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, NULL);

		static std::map<ImGuiID, tab_state> anim;
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, tab_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.text = ImLerp(it_anim->second.text,
			selectable ? ImColor(255, 255, 255, 255) :
			hovered ? ImColor(100, 100, 100, 255) :
			ImColor(59, 62, 72, 255),
			g.IO.DeltaTime * 7.5f);

		it_anim->second.text_offset = ImClamp(it_anim->second.text_offset + (6.f * g.IO.DeltaTime * (selectable ? 10.f : -10.f)), 0.f, 24.f);
		it_anim->second.arrow_opticaly = ImLerp(it_anim->second.arrow_opticaly, selectable ? 1.f : 0.f, g.IO.DeltaTime * 7.5f);

		GetWindowDrawList()->AddCircleFilled(ImVec2(bb.Min.x + 8.f, bb.Min.y + 10.f), 3.f, GetColorU32(accent_color, it_anim->second.arrow_opticaly), 100.f);

		ImVec2 icon_pos = ImVec2(bb.Min.x + it_anim->second.text_offset, bb.Max.y - icon_size.y - (size.y - icon_size.y) / 2);
		GetWindowDrawList()->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, icon_pos, GetColorU32(it_anim->second.text), icon);

		ImVec2 label_pos = ImVec2(icon_pos.x + icon_size.x + 6.f, bb.Max.y - label_size.y - (size.y - label_size.y) / 2);
		GetWindowDrawList()->AddText(label_pos, GetColorU32(it_anim->second.text), label);

		return pressed;
	}

	inline bool SubTabBeta(bool selectable, const char* icon, const char* label, const ImVec2& size_arg)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImVec2 icon_size = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, icon);
		const ImVec2 pos = window->DC.CursorPos;

		ImVec2 size = CalcItemSize(size_arg, label_size.x + icon_size.x + 8.0f, ImMax(label_size.y, icon_size.y));
		const ImRect bb(pos, pos + size);

		ImGui::ItemSize(size, 0.f);
		if (!ImGui::ItemAdd(bb, id)) return false;

		bool hovered, held, pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, NULL);

		static std::map<ImGuiID, tab_state> anim;
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, tab_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.text = ImLerp(it_anim->second.text,
			selectable ? ImColor(135, 131, 142, 255) :
			hovered ? ImColor(100, 100, 100, 255) :
			ImColor(59, 62, 72, 255),
			g.IO.DeltaTime * 7.5f);

		it_anim->second.text_offset = ImClamp(it_anim->second.text_offset + (6.f * g.IO.DeltaTime * (selectable ? 10.f : -10.f)), 0.f, 24.f);
		it_anim->second.arrow_opticaly = ImLerp(it_anim->second.arrow_opticaly, selectable ? 1.f : 0.f, g.IO.DeltaTime * 7.5f);

		ImVec2 arrow_center = ImVec2(bb.Min.x + 8.f, bb.Min.y + 10.f);
		float size_arrow = 5.0f; // Tamanho da seta
		ImVec2 p1 = ImVec2(arrow_center.x - size_arrow * 0.5f, arrow_center.y - size_arrow * 0.5f);
		ImVec2 p2 = ImVec2(arrow_center.x - size_arrow * 0.5f, arrow_center.y + size_arrow * 0.5f);
		ImVec2 p3 = ImVec2(arrow_center.x + size_arrow * 0.5f, arrow_center.y);
		GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, GetColorU32(accent_color, it_anim->second.arrow_opticaly));


		ImVec2 icon_pos = ImVec2(bb.Min.x + it_anim->second.text_offset, bb.Max.y - icon_size.y - (size.y - icon_size.y) / 2);
		GetWindowDrawList()->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, icon_pos, GetColorU32(it_anim->second.text), icon);

		ImVec2 label_pos = ImVec2(icon_pos.x + icon_size.x + 6.f, bb.Max.y - label_size.y - (size.y - label_size.y) / 2);
		GetWindowDrawList()->AddText(label_pos, GetColorU32(it_anim->second.text), label);

		return pressed;
	}

	inline bool SubTab(const char* label, bool active)
	{
		struct SubTab_t {
			ImVec4 BackgroundColor;
			ImVec2 BackgroundGrow;
			ImVec4 TextColor;
			float UnSelectedAnim;
		};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* DrawList = window->DrawList;

		if (window->SkipItems) {
			return false;
		}

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		std::string IdStr = std::string(label);
		const ImGuiID id = window->GetID(IdStr.c_str());
		const ImGuiIO IO = g.IO;

		const ImVec2 pos = window->DC.CursorPos;

		auto TextSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, label);

		const ImRect rect(pos, pos + ImVec2(TextSize.x + 16, 30));
		ImGui::ItemSize(rect, style.FramePadding.y);
		if (!ImGui::ItemAdd(rect, id)) {
			return false;
		}

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
		if (pressed) { ImGui::MarkItemEdited(id); }

		static std::map<ImGuiID, SubTab_t> anim;
		auto SubTabAnim = anim.find(id);

		if (SubTabAnim == anim.end()) {
			anim.insert({ id, SubTab_t() });
			SubTabAnim = anim.find(id);
		}

		float NormalizedTime = ImClamp(IO.DeltaTime * 10.f, 0.0f, 1.0f);
		float NormalizedTime2 = ImClamp(IO.DeltaTime * 8.f, 0.0f, 1.0f);
		SubTabAnim->second.BackgroundColor = ImLerp(SubTabAnim->second.BackgroundColor, active ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 30.f / 255.f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f), Custom::EaseInOutCirc(NormalizedTime));
		SubTabAnim->second.TextColor = ImLerp(SubTabAnim->second.TextColor, active ? ImColor( /*g_Col.Base*/ 255, 255, 255) : ImColor(60, 60, 60), Custom::EaseInOutCirc(NormalizedTime));
		SubTabAnim->second.BackgroundGrow = ImLerp(SubTabAnim->second.BackgroundGrow, active ? ImVec2(4, 4) : ImVec2(0, 0), Custom::EaseInOutCirc(NormalizedTime2));
		SubTabAnim->second.UnSelectedAnim = ImLerp(SubTabAnim->second.UnSelectedAnim, hovered && !active ? 2.f : 0.f, IO.DeltaTime * 4.f);

		const float rounding = 12.f;
		DrawList->AddRectFilled(ImVec2(rect.Min.x - SubTabAnim->second.BackgroundGrow.x, rect.Min.y - SubTabAnim->second.BackgroundGrow.x), ImVec2(rect.Max.x + SubTabAnim->second.BackgroundGrow.x, rect.Max.y + SubTabAnim->second.BackgroundGrow.x), ImGui::GetColorU32(SubTabAnim->second.BackgroundColor), rounding, 0);

		DrawList->AddText(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, ImVec2(pos.x + ((rect.GetWidth() - TextSize.x) / 2), pos.y + ((rect.GetHeight() - TextSize.y) / 2) - 1), ImGui::GetColorU32(SubTabAnim->second.TextColor), label);


		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	}

	inline bool SubTabVertical(const char* label, bool active)
	{
		struct SubTab_t {
			ImVec4 BackgroundColor;
			ImVec2 BackgroundGrow;
			ImVec4 TextColor;
			float UnSelectedAnim;
		};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* DrawList = window->DrawList;

		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		std::string IdStr = std::string(label);
		const ImGuiID id = window->GetID(IdStr.c_str());
		const ImGuiIO& IO = g.IO;

		const ImVec2 pos = window->DC.CursorPos;

		auto TextSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, label);

		// Alterar tamanho para subtab vertical (mais alta que larga)
		const float tabWidth = 80.f;
		const float tabHeight = TextSize.y + 16;
		const ImRect rect(pos, pos + ImVec2(tabWidth, tabHeight));
		ImGui::ItemSize(rect, style.FramePadding.y);
		if (!ImGui::ItemAdd(rect, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
		if (pressed) ImGui::MarkItemEdited(id);

		static std::map<ImGuiID, SubTab_t> anim;
		auto& SubTabAnim = anim[id]; // Cria ou acessa direto

		float NormalizedTime = ImClamp(IO.DeltaTime * 10.f, 0.0f, 1.0f);
		float NormalizedTime2 = ImClamp(IO.DeltaTime * 8.f, 0.0f, 1.0f);

		SubTabAnim.BackgroundColor = ImLerp(SubTabAnim.BackgroundColor,
			active ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 30.f / 255.f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f),
			Custom::EaseInOutCirc(NormalizedTime));

		SubTabAnim.TextColor = ImLerp(SubTabAnim.TextColor,
			active ? ImColor(255, 255, 255) : ImColor(60, 60, 60),
			Custom::EaseInOutCirc(NormalizedTime));

		SubTabAnim.BackgroundGrow = ImLerp(SubTabAnim.BackgroundGrow, active ? ImVec2(3, 3) : ImVec2(0, 0), Custom::EaseInOutCirc(NormalizedTime2));
		SubTabAnim.UnSelectedAnim = ImLerp(SubTabAnim.UnSelectedAnim, hovered && !active ? 2.f : 0.f, IO.DeltaTime * 4.f);

		const float rounding = 8.f;
		DrawList->AddRectFilled(
			ImVec2(rect.Min.x - SubTabAnim.BackgroundGrow.x, rect.Min.y - SubTabAnim.BackgroundGrow.y),
			ImVec2(rect.Max.x + SubTabAnim.BackgroundGrow.x, rect.Max.y + SubTabAnim.BackgroundGrow.y),
			ImGui::GetColorU32(SubTabAnim.BackgroundColor),
			rounding, ImDrawFlags_RoundCornersAll);

		// Centraliza o texto horizontalmente e verticalmente
		DrawList->AddText(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize,
			ImVec2(
				pos.x + (tabWidth - TextSize.x) / 2,
				pos.y + (tabHeight - TextSize.y) / 2 - 1),
			ImGui::GetColorU32(SubTabAnim.TextColor),
			label);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	}


	inline bool Tab(bool selected, const char* icon_label, const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
		ImVec2 pos = window->DC.CursorPos;

		const ImRect rect(pos, pos + size_arg);
		ImGui::ItemSize(rect, style.FramePadding.y);
		if (!ImGui::ItemAdd(rect, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, flags);

		static std::map<ImGuiID, tab_state> anim;
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, tab_state() });
			it_anim = anim.find(id);
		}

		float normTime = ImClamp(g.IO.DeltaTime * 15.f, 0.f, 1.f);

		// Anima alpha da linha esquerda (vermelha)
		it_anim->second.leftLineAlpha = ImLerp(it_anim->second.leftLineAlpha, selected ? 1.f : 0.f, normTime);

		// Anima cor do texto (branco para ativo, cinza escuro para inativo)
		it_anim->second.text = ImLerp(it_anim->second.text,
			selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(60.f / 255.f, 60.f / 255.f, 60.f / 255.f, 1.f),
			normTime);

		// Desenha fundo transparente (combina com a sidebar)
		ImU32 bg_col;
		if (hovered) {
			bg_col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, selected ? 0.06f : 0.03f));
		} else {
			bg_col = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.f));
		}
		window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_col, 6.0f);

		// Linha ciano fina na esquerda (accent)
		ImU32 lineColor = ImGui::GetColorU32(ImVec4(1.f, 212.f / 255.f, 0.f, it_anim->second.leftLineAlpha));
		window->DrawList->AddLine(
			ImVec2(rect.Min.x + 2, rect.Min.y + 4),
			ImVec2(rect.Min.x + 2, rect.Max.y - 4),
			lineColor,
			2.0f
		);

		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(it_anim->second.text));
		ImGui::RenderTextClipped(rect.Min + ImVec2(12, 12), rect.Max + ImVec2(0, 12), label, nullptr, &label_size);
		ImGui::PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	}

	inline bool CustomChild(const char* text, const char* id, const ImVec2& size)
	{
		PushStyleColor(ImGuiCol_ChildBg, ImColor(17, 17, 20, (int)(GetStyle().Alpha * 255)).Value);
		PushStyleVar(ImGuiStyleVar_ChildRounding, 5);

		GetWindowDrawList()->AddRectFilled(GetWindowPos() + GetCursorPos(), GetWindowPos() + GetCursorPos() + ImVec2(size.x, 35), ImColor(27, 29, 32, (int)(GetStyle().Alpha * 255)), 5, ImDrawFlags_RoundCornersTop);
		ImGui::PushFont(g_Variables.lexend_font);
		GetWindowDrawList()->AddText(GetWindowPos() + GetCursorPos() + ImVec2(10, 10), ImColor(200, 200, 200), text);
		PopFont();
		GetWindowDrawList()->AddRect(GetWindowPos() + GetCursorPos(), GetWindowPos() + GetCursorPos() + ImVec2(size.x, 36), GetColorU32(ImGuiCol_Border), GetStyle().ChildRounding, ImDrawFlags_RoundCornersTop);

		SetCursorPosY(GetCursorPosY() + 35);
		bool ret = BeginChild(id, size - ImVec2(0, 35), false);
		SetCursorPos(ImVec2(10, 10));
		BeginGroup();

		return ret;
	}

	inline void EndCustomChild()
	{
		if (GetCurrentWindow()->ScrollbarY > 0)
			SetCursorPosY(GetCursorPosY() + 10);

		EndGroup();
		GetWindowDrawList()->AddRect(GetWindowPos(), GetWindowPos() + GetWindowSize(), GetColorU32(ImGuiCol_Border), GetStyle().ChildRounding, ImDrawFlags_RoundCornersBottom);
		EndChild();
		PopStyleVar();
		PopStyleColor();
		SetCursorPosY(GetCursorPosY() + 12.5f);
	}

	inline bool ChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* parent_window = g.CurrentWindow;

		flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
		flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);

		const ImVec2 content_avail = GetContentRegionAvail();
		ImVec2 size = ImTrunc(size_arg);
		const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
		if (size.x <= 0.0f) size.x = ImMax(content_avail.x + size.x, 4.0f);
		if (size.y <= 0.0f) size.y = ImMax(content_avail.y + size.y, 4.0f);

		SetNextWindowPos(ImVec2(parent_window->DC.CursorPos + ImVec2(0, cap ? 45 : 0)));
		SetNextWindowSize(size - ImVec2(0, cap ? 45 : 0));

		GetWindowDrawList()->AddRectFilled(parent_window->DC.CursorPos + ImVec2(0, cap ? 45 : 0), parent_window->DC.CursorPos + size_arg, GetColorU32(c::child::background), c::child::rounding, cap ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersAll);

		if (cap) {

			GetWindowDrawList()->AddRectFilledMultiColor(parent_window->DC.CursorPos + ImVec2((size_arg.x / 2), 44), parent_window->DC.CursorPos + ImVec2((size_arg.x - 50), 45), GetColorU32(c::accent), GetColorU32(c::accent, 0.f), GetColorU32(c::accent, 0.f), GetColorU32(c::accent), c::child::rounding);
			GetWindowDrawList()->AddRectFilledMultiColor(parent_window->DC.CursorPos + ImVec2(50, 44), parent_window->DC.CursorPos + ImVec2(size_arg.x / 2, 45), GetColorU32(c::accent, 0.f), GetColorU32(c::accent), GetColorU32(c::accent), GetColorU32(c::accent, 0.f), c::child::rounding);

			GetWindowDrawList()->AddRectFilled(parent_window->DC.CursorPos, parent_window->DC.CursorPos + ImVec2(size_arg.x, 35), GetColorU32(c::child::cap), c::child::rounding, ImDrawFlags_RoundCornersTop);
			GetWindowDrawList()->AddText(parent_window->DC.CursorPos + ImVec2(size_arg.x - CalcTextSize(name).x, 35 - CalcTextSize(name).y) / 2, GetColorU32(c::text::text_active), name);
		}

		const char* temp_window_name;

		if (name) ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);

		else ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

		const float backup_border_size = g.Style.ChildBorderSize;

		bool ret = Begin(temp_window_name, NULL, flags | ImGuiWindowFlags_NoBackground);

		ImGuiWindow* child_window = g.CurrentWindow;
		child_window->ChildId = id;
		child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

		if (child_window->BeginCount == 1) parent_window->DC.CursorPos = child_window->Pos;

		const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
		if (g.ActiveId == temp_id_for_activation) ClearActiveID();

		if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
		{
			FocusWindow(child_window);
			NavInitWindow(child_window, false);
			SetActiveID(temp_id_for_activation, child_window);
			g.ActiveIdSource = g.NavInputSource;
		}
		return ret;
	}

	inline bool Child(const char* str_id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags extra_flags)
	{
		ImGuiWindow* window = GetCurrentWindow();

		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13, 13));
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(13, 13));

		return ChildEx(str_id, window->GetID(str_id), size_arg, cap, extra_flags | ImGuiWindowFlags_AlwaysUseWindowPadding);
	}

	inline void EndChild2()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		PopStyleVar(2);

		IM_ASSERT(g.WithinEndChild == false);
		IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);

		g.WithinEndChild = true;
		if (window->BeginCount > 1)
		{
			End();
		}
		else
		{
			ImVec2 sz = window->Size;

			if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) sz.x = ImMax(4.0f, sz.x);
			if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y)) sz.y = ImMax(4.0f, sz.y);

			End();

			ImGuiWindow* parent_window = g.CurrentWindow;
			ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
			ItemSize(sz);
			if ((window->DC.NavLayersActiveMask != 0 || window->DC.NavWindowHasScrollY) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
			{
				ItemAdd(bb, window->ChildId);
			}
			else
			{
				ItemAdd(bb, 0);

				if (window->Flags & ImGuiWindowFlags_NavFlattened) parent_window->DC.NavLayersActiveMaskNext |= window->DC.NavLayersActiveMaskNext;
			}
			if (g.HoveredWindow == window) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
		}
		g.WithinEndChild = false;
		g.LogLinePosY = -FLT_MAX;
	}

	inline void InvisibleShadow(const float alpha, const ImVec2 pos_min, const ImVec2 pos_max, ImU32 color, float shadow_tickness)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		ImGui::PushClipRect(pos_min, pos_max, true);
		ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(ImGui::GetMousePos()), 1.f, ImGui::GetColorU32(color), shadow_tickness, ImVec2(0, 0), 1000.f);
		ImGui::PopClipRect();
		ImGui::PopStyleVar();
	}

	inline void ToolTip(const char* Label, const char* Desc, const char* Icon, bool State) {
		//bool Check = CheckBox( Label, Checked );
		bool Hovered = State;

		ImVec2 Pos = ImGui::GetMousePos() + ImVec2(20, 0);
		auto DrawList = ImGui::GetForegroundDrawList();
		float Padding = 12;
		float Rounding = 4;
		float Spacing = 6;

		struct ToolTip_t {
			ImVec4 BackGroundColor = ImColor(20, 20, 22);
			ImVec4 TextColor = ImColor(200, 200, 200, 0);
			ImVec4 IconColor = ImColor(245, 158, 66, 0);

			float GlobalAlpha = 0.f;
			float SlideX = 0.f;
		};

		static std::map<std::string, ToolTip_t> anim;
		std::string a1 = std::string(Label) + std::string(Desc);
		auto ToolTipAnim = anim.find(a1);

		if (ToolTipAnim == anim.end()) {
			anim.insert({ a1, ToolTip_t() });
		}

		ImVec2 TextSize = Utils::CalcTextSize(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, Desc);
		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, Icon);

		ToolTipAnim->second.GlobalAlpha = ImLerp(ToolTipAnim->second.GlobalAlpha, Hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 8);
		ToolTipAnim->second.SlideX = ImLerp(ToolTipAnim->second.SlideX, Hovered ? (IconTextSize.x + TextSize.x + Padding + Spacing) : 0.f, ImGui::GetIO().DeltaTime * 8);
		ToolTipAnim->second.TextColor = ImLerp(ToolTipAnim->second.TextColor, Hovered ? ImColor(180, 180, 180) : ImColor(180, 180, 180, 0), ImGui::GetIO().DeltaTime * 12);
		ToolTipAnim->second.IconColor = ImLerp(ToolTipAnim->second.IconColor, Hovered ? ImVec4(g_Col.Base) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f / 255.f), ImGui::GetIO().DeltaTime * 8);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ToolTipAnim->second.GlobalAlpha);


		ImVec2 RectEndPos = ImVec2(Pos.x + ToolTipAnim->second.SlideX, Pos.y + TextSize.y + Padding);
		DrawList->AddRectFilled(Pos, RectEndPos, ImGui::GetColorU32(ImVec4(ImColor(16, 16, 18))), Rounding);
		DrawList->AddRect(Pos, RectEndPos, ImGui::GetColorU32(ImVec4(ImColor(22, 22, 24))), Rounding);


		DrawList->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ImVec2(Pos.x + Padding / 2, Pos.y + ((TextSize.y + Padding) / 2 - IconTextSize.y / 2)), ImGui::GetColorU32(ToolTipAnim->second.IconColor), Icon);

		if (ToolTipAnim->second.SlideX < TextSize.x + Padding) {

			//ChatGPT patrocina
			std::string ClippedDesc = Desc;
			while (ImGui::CalcTextSize(ClippedDesc.c_str()).x > ToolTipAnim->second.SlideX - Padding && !ClippedDesc.empty()) {
				ClippedDesc.pop_back();
			}

			DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ImVec2(Pos.x + Padding / 2 + (IconTextSize.x + Spacing), Pos.y + Padding / 2), ImGui::GetColorU32(ToolTipAnim->second.TextColor), ClippedDesc.c_str());
		}
		else {
			DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ImVec2(Pos.x + Padding / 2 + (IconTextSize.x + Spacing), Pos.y + Padding / 2), ImGui::GetColorU32(ToolTipAnim->second.TextColor), Desc);
		}

		ImGui::PopStyleVar();

		//return Check;
	}

	// Shared toggle-switch renderer (thin track + light knob, animated with ImLerp).
	// Keeps the exact bool* contract of the old square checkboxes.
	inline float ToggleAnim(ImGuiID id, bool on) {
		static std::map<ImGuiID, float> m;
		float& t = m[id];
		t = ImLerp(t, on ? 1.f : 0.f, ImClamp(ImGui::GetIO().DeltaTime * 12.f, 0.f, 1.f));
		return t;
	}

	inline void DrawToggleSwitch(ImDrawList* dl, ImVec2 track_min, float t, bool hovered) {
		const float tw = 32.f, th = 17.f;
		ImVec4 off = hovered ? ImVec4(0x33 / 255.f, 0x33 / 255.f, 0x33 / 255.f, 1.f)
			: ImVec4(0x29 / 255.f, 0x29 / 255.f, 0x29 / 255.f, 1.f); // graphite OFF
		ImVec4 on = hovered ? ImVec4(1.f, 224.f / 255.f, 51.f / 255.f, 1.f)
			: ImVec4(1.f, 212.f / 255.f, 0.f, 1.f); // yellow ON
		ImVec4 track = ImLerp(off, on, t);
		dl->AddRectFilled(track_min, track_min + ImVec2(tw, th), ImGui::GetColorU32(track), th * 0.5f);
		float kr = (th - 4.f) * 0.5f;
		float kx = ImLerp(track_min.x + 2.f + kr, track_min.x + tw - 2.f - kr, t);
		ImVec4 knob = ImLerp(ImVec4(0.96f, 0.96f, 0.96f, 1.f), ImVec4(0x0A / 255.f, 0x0A / 255.f, 0x0A / 255.f, 1.f), t);
		dl->AddCircleFilled(ImVec2(kx, track_min.y + th * 0.5f), kr, ImGui::GetColorU32(knob), 20);
	}

	inline bool CheckBox2(const char* Label, bool* Checked, bool bToolTip = false, const char* ToolTipMsg = "", const char* ToolTipIcon = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems || !Window->Active || Window->Hidden)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = std::string(Label) + std::to_string(reinterpret_cast<uintptr_t>(Checked));
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		const ImVec2 CheckBoxSize(22, 22);
		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		const float spacing = 8.0f;

		ImVec2 labelPos = Window->DC.CursorPos;

		float contentRight = ImGui::GetWindowContentRegionMax().x + Window->Pos.x;
		ImVec2 checkPos = ImVec2(contentRight - CheckBoxSize.x, Window->DC.CursorPos.y);
		ImRect checkRect(checkPos, checkPos + CheckBoxSize);

		float fullHeight = (CheckBoxSize.y > TextSize.y) ? CheckBoxSize.y : TextSize.y;
		ImVec2 clickableMin = labelPos;
		ImVec2 clickableMax = ImVec2(contentRight, labelPos.y + fullHeight);
		ImRect clickable(clickableMin, clickableMax);

		ImGui::ItemSize(clickable, style.FramePadding.y);
		if (!ImGui::ItemAdd(clickable, id)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(255, 212, 0);
			ImVec4 LabelColor = g_Col.SecundaryText;
			ImVec2 BackGroundSize = ImVec2(0, 0);
			float CheckUp = 0.f;
			ImVec4 IconColor = ImColor(255, 212, 0, 0);
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto& animData = anim[id];

		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ICON_FA_TRIANGLE_EXCLAMATION);

		animData.IconColor = ImLerp(animData.IconColor, Hovered || *Checked ? ImVec4(g_Col.Base) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f / 255.f), ImGui::GetIO().DeltaTime * 12);
		animData.LabelColor = ImLerp(animData.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		animData.BackGroundColor = ImLerp(animData.BackGroundColor, *Checked ? ImColor(255, 212, 0) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		animData.UnCheckedBackGroundColor = ImLerp(animData.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		animData.BackGroundSize = ImLerp(animData.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (animData.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			animData.CheckColor = ImLerp(animData.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			animData.CheckUp = ImLerp(animData.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			animData.CheckColor = ImLerp(animData.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			animData.CheckUp = ImLerp(animData.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		float tgl = ToggleAnim(id, *Checked);
		ImVec2 track_min = ImVec2(checkRect.Max.x - 30.f, checkRect.Min.y + (checkRect.GetHeight() - 16.f) * 0.5f);
		DrawToggleSwitch(Window->DrawList, track_min, tgl, Hovered);

		Window->DrawList->AddText(labelPos, ImGui::GetColorU32(animData.LabelColor), Label);

		// ================= REMOVIDA A LINHA DE SEPARAï¿½ï¿½O =================
		// if (g_MenuInfo.IsOpen) {
		//     float offsetY = 6.0f;
		//     Window->DrawList->AddLine(ImVec2(labelPos.x, checkRect.Max.y + offsetY), ImVec2(contentRight, checkRect.Max.y + offsetY), (ImU32)ImColor(30, 30, 30), 1.0f);
		// }

		if (bToolTip && g_MenuInfo.IsOpen && ImGui::GetStyle().Alpha > 0.01f) {
			ImVec4 warnColor = ImColor(255, 212, 0);
			warnColor.w = ImGui::GetStyle().Alpha;

			ImFont* iconFont = g_Variables.FontAwesomeSolid;
			float iconSize = iconFont->FontSize;

			ImVec2 iconPos = ImVec2(checkPos.x - iconSize - 6.0f, checkPos.y + (CheckBoxSize.y - iconSize) * 0.5f);

			ImGui::PushFont(iconFont);
			Window->DrawList->AddText(iconFont, iconSize, iconPos, ImGui::GetColorU32(warnColor), ICON_FA_TRIANGLE_EXCLAMATION);
			ImGui::PopFont();
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}


	inline bool CheckBox(const char* Label, bool* Checked, bool bToolTip = false, const char* ToolTipMsg = "", const char* ToolTipIcon = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems || !Window->Active || Window->Hidden)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = std::string(Label) + std::to_string(reinterpret_cast<uintptr_t>(Checked));
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		const ImVec2 CheckBoxSize(22, 22);
		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		const float spacing = 8.0f;

		ImVec2 labelPos = Window->DC.CursorPos;

		float contentRight = ImGui::GetWindowContentRegionMax().x + Window->Pos.x;
		ImVec2 checkPos = ImVec2(contentRight - CheckBoxSize.x, Window->DC.CursorPos.y);
		ImRect checkRect(checkPos, checkPos + CheckBoxSize);

		float fullHeight = (CheckBoxSize.y > TextSize.y) ? CheckBoxSize.y : TextSize.y;
		ImVec2 clickableMin = labelPos;
		ImVec2 clickableMax = ImVec2(contentRight, labelPos.y + fullHeight);
		ImRect clickable(clickableMin, clickableMax);

		ImGui::ItemSize(clickable, style.FramePadding.y);
		if (!ImGui::ItemAdd(clickable, id)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(255, 212, 0);
			ImVec4 LabelColor = g_Col.SecundaryText;
			ImVec2 BackGroundSize = ImVec2(0, 0);
			float CheckUp = 0.f;
			ImVec4 IconColor = ImColor(255, 212, 0, 0);
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto& animData = anim[id];

		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ICON_FA_TRIANGLE_EXCLAMATION);

		animData.IconColor = ImLerp(animData.IconColor, Hovered || *Checked ? ImVec4(g_Col.Base) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f / 255.f), ImGui::GetIO().DeltaTime * 12);
		animData.LabelColor = ImLerp(animData.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		animData.BackGroundColor = ImLerp(animData.BackGroundColor, *Checked ? ImColor(255, 212, 0) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		animData.UnCheckedBackGroundColor = ImLerp(animData.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		animData.BackGroundSize = ImLerp(animData.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (animData.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			animData.CheckColor = ImLerp(animData.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			animData.CheckUp = ImLerp(animData.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			animData.CheckColor = ImLerp(animData.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			animData.CheckUp = ImLerp(animData.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		float tgl = ToggleAnim(id, *Checked);
		ImVec2 track_min = ImVec2(checkRect.Max.x - 30.f, checkRect.Min.y + (checkRect.GetHeight() - 16.f) * 0.5f);
		DrawToggleSwitch(Window->DrawList, track_min, tgl, Hovered);

		Window->DrawList->AddText(labelPos, ImGui::GetColorU32(animData.LabelColor), Label);


		if (bToolTip && g_MenuInfo.IsOpen && ImGui::GetStyle().Alpha > 0.01f) {
			ImVec4 warnColor = ImColor(255, 212, 0);
			warnColor.w = ImGui::GetStyle().Alpha;

			ImFont* iconFont = g_Variables.FontAwesomeSolid;
			float iconSize = iconFont->FontSize;

			ImVec2 iconPos = ImVec2(checkPos.x - iconSize - 6.0f, checkPos.y + (CheckBoxSize.y - iconSize) * 0.5f);

			ImGui::PushFont(iconFont);
			Window->DrawList->AddText(iconFont, iconSize, iconPos, ImGui::GetColorU32(warnColor), ICON_FA_TRIANGLE_EXCLAMATION);
			ImGui::PopFont();
		}



		IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}

	static float CalcMaxPopupHeightFromItemCount(int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

	struct ComboAnim
	{
		float a;
		float b;
	};


	struct ComboPopupAnim
	{
		float a;
	};

	inline bool BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags, int itemcount)
	{
		static std::unordered_map<ImGuiID, ComboPopupAnim> Values;
		auto Value = Values.find(popup_id);

		if (Value == Values.end())
		{
			Values.insert({ popup_id, ComboPopupAnim() });
			Value = Values.find(popup_id);
		}

		bool awffawfaf = IsPopupOpen(popup_id, ImGuiPopupFlags_None);

		Value->second.a = ImLerp(Value->second.a, awffawfaf ? 1.f : 0.f, 0.1f);

		ImGuiContext& g = *GImGui;
		if (!awffawfaf)
		{
			g.NextWindowData.ClearFlags();
			return false;
		}

		// Set popup size
		float w = bb.GetWidth();

		SetNextWindowSize(ImVec2(w, (CalcMaxPopupHeightFromItemCount(itemcount) + 21) * Value->second.a));

		// This is essentially a specialized version of BeginPopupEx()
		char name[16];
		ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

		if (ImGuiWindow* popup_window = FindWindowByName(name))
			if (popup_window->WasActive)
			{
				// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
				ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
				popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
				ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
				ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
				SetNextWindowPos(pos + ImVec2(0, 2));
			}

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
		PushStyleColor(ImGuiCol_PopupBg, ImColor(14, 14, 14).Value);
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10)); // Horizontally align ourselves with the framed text
		PushStyleVar(ImGuiStyleVar_PopupRounding, 5); // Horizontally align ourselves with the framed text
		bool ret = Begin(name, NULL, window_flags);
		PopStyleVar(2);
		PopStyleColor();
		if (!ret)
		{
			EndPopup();
			IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
			return false;
		}

		return true;
	}

	inline bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags, int itemcount)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();

		ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
		g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
		if (window->SkipItems)
			return false;


		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
		if (flags & ImGuiComboFlags_WidthFitPreview)
			IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | ImGuiComboFlags_CustomPreview)) == 0);

		const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const float preview_width = CalcTextSize(preview_value, NULL, true).x;

		static std::unordered_map<ImGuiID, ComboAnim> Values;
		auto Value = Values.find(id);

		if (Value == Values.end())
		{
			Values.insert({ id, ComboAnim() });
			Value = Values.find(id);
		}



		Value->second.b = ImLerp(Value->second.b, 19 + preview_width + 14, 0.1f);

		const ImRect bb(window->DC.CursorPos + ImVec2(GetWindowSize().x - 20 - Value->second.b, 0), window->DC.CursorPos + ImVec2(GetWindowSize().x - 20, 25));
		const ImRect total_bb(window->DC.CursorPos, bb.Max);

		PopFont();

		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &bb))
			return false;


		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held);
		const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
		bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
		if (pressed && !popup_open)
		{
			OpenPopupEx(popup_id, ImGuiPopupFlags_None);
			popup_open = true;
		}

		Value->second.a = ImLerp(Value->second.a, popup_open ? 3.f : 2.f, 0.1f);

		// Render shape
		const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
		RenderNavHighlight(bb, id);
		if (!(flags & ImGuiComboFlags_NoPreview))
		{
			window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(14, 14, 14, (int)(GetStyle().Alpha * 255)), 5);
			window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), 5);
			window->DrawList->AddCircleFilled(ImVec2(bb.Max.x - 14, bb.Min.y + total_bb.GetHeight() / 2), Value->second.a, ImColor(255, 212, 0, (int)(GetStyle().Alpha * 255)), 360);
		}


		if (flags & ImGuiComboFlags_CustomPreview)
		{
			g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
			IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
			preview_value = NULL;
		}

		if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
		{
			if (g.LogEnabled)
				LogSetNextTextDecoration("{", "}");

			RenderTextClipped(bb.Min + ImVec2(7, bb.GetHeight() / 2 - label_size.y / 2), bb.Max, preview_value, NULL, NULL);
		}

		if (label_size.x > 0)
			GetWindowDrawList()->AddText(total_bb.Min + ImVec2(0, total_bb.GetHeight() / 2 - label_size.y / 2), ImColor(1.f, 1.f, 1.f, 0.8f * GetStyle().Alpha), label);

		PopFont();

		g.NextWindowData.Flags = backup_next_window_data_flags;
		return BeginComboPopup(popup_id, bb, flags, itemcount);
	}


	inline void EndCombo()
	{
		EndPopup();
	}

	inline bool BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		const ImGuiID id = window->GetID(name);
		if (!IsPopupOpen(id, ImGuiPopupFlags_None))
		{
			g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
			if (p_open && *p_open)
				*p_open = false;
			return false;
		}

		// Center modal windows by default for increased visibility
		// (this won't really last as settings will kick in, and is mostly for backward compatibility. user may do the same themselves)
		// FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
		if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
		{
			const ImGuiViewport* viewport = GetMainViewport();
			SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		}

		flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse;
		const bool is_open = Begin(name, p_open, flags);
		if (!is_open || (p_open && !*p_open)) // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
		{
			EndPopup();
			if (is_open)
				ClosePopupToLevel(g.BeginPopupStack.Size, true);
			return false;
		}
		return is_open;
	}

	inline bool EndPopup()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		IM_ASSERT(window->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
		IM_ASSERT(g.BeginPopupStack.Size > 0);

		// Make all menus and popups wrap around for now, may need to expose that policy (e.g. focus scope could include wrap/loop policy flags used by new move requests)
		if (g.NavWindow == window)
			NavMoveRequestTryWrapping(window, ImGuiNavMoveFlags_LoopY);

		// Child-popups don't need to be laid out
		IM_ASSERT(g.WithinEndChild == false);
		if (window->Flags & ImGuiWindowFlags_ChildWindow)
			g.WithinEndChild = true;
		End();
		g.WithinEndChild = false;
	}

	// Call directly after the BeginCombo/EndCombo block. The preview is designed to only host non-interactive elements
	// (Experimental, see GitHub issues: #1658, #4168)
	inline bool BeginComboPreview()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

		if (window->SkipItems || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible))
			return false;
		IM_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y); // Didn't call after BeginCombo/EndCombo block or forgot to pass ImGuiComboFlags_CustomPreview flag?
		if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) // Narrower test (optional)
			return false;

		// FIXME: This could be contained in a PushWorkRect() api
		preview_data->BackupCursorPos = window->DC.CursorPos;
		preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
		preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
		preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
		preview_data->BackupLayout = window->DC.LayoutType;
		window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
		window->DC.CursorMaxPos = window->DC.CursorPos;
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.IsSameLine = false;
		PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

		return true;
	}

	inline bool EndComboPreview()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

		// FIXME: Using CursorMaxPos approximation instead of correct AABB which we will store in ImDrawCmd in the future
		ImDrawList* draw_list = window->DrawList;
		if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
			if (draw_list->CmdBuffer.Size > 1) // Unlikely case that the PushClipRect() didn't create a command
			{
				draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
				draw_list->_TryMergeDrawCmds();
			}
		PopClipRect();
		window->DC.CursorPos = preview_data->BackupCursorPos;
		window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
		window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
		window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
		window->DC.LayoutType = preview_data->BackupLayout;
		window->DC.IsSameLine = false;
		preview_data->PreviewRect = ImRect();
	}

	// Getter for the old Combo() API: const char*[]
	static const char* Items_ArrayGetter(void* data, int idx)
	{
		const char* const* items = (const char* const*)data;
		return items[idx];
	}

	// Getter for the old Combo() API: "item1\0item2\0item3\0"
	static const char* Items_SingleStringGetter(void* data, int idx)
	{
		const char* items_separated_by_zeros = (const char*)data;
		int items_count = 0;
		const char* p = items_separated_by_zeros;
		while (*p)
		{
			if (idx == items_count)
				break;
			p += strlen(p) + 1;
			items_count++;
		}
		return *p ? p : NULL;
	}

	inline bool Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
	{
		ImGuiContext& g = *GImGui;

		// Call the getter to obtain the preview string which is a parameter to BeginCombo()
		const char* preview_value = NULL;
		if (*current_item >= 0 && *current_item < items_count)
			preview_value = getter(user_data, *current_item);

		// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
		if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
			SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

		if (!BeginCombo(label, preview_value, ImGuiComboFlags_None, items_count))
			return false;


		// Display items
		// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
		bool value_changed = false;
		for (int i = 0; i < items_count; i++)
		{
			const char* item_text = getter(user_data, i);
			if (item_text == NULL)
				item_text = "*Unknown item*";

			PushID(i);
			const bool item_selected = (i == *current_item);
			if (Selectable(item_text, item_selected) && *current_item != i)
			{
				value_changed = true;
				*current_item = i;
			}
			if (item_selected)
				SetItemDefaultFocus();
			PopID();
		}

		EndCombo();

		if (value_changed)
			MarkItemEdited(g.LastItemData.ID);

		return value_changed;
	}

	inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
	{
		const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
		return value_changed;
	}

	// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
	inline bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
	{
		int items_count = 0;
		const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
		while (*p)
		{
			p += strlen(p) + 1;
			items_count++;
		}
		bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
		return value_changed;
	}


	inline bool CheckBoxCfg(const char* Label, bool* Checked, std::function<void()> Components, bool bToolTip = false, const char* ToolTipMsg = "", const char* ToolTipIcon = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = std::string(Label) + std::to_string(reinterpret_cast<uintptr_t>(Checked));
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		const float CheckBoxSizeVal = 22.f;
		const ImVec2 CheckBoxSize(30.f, CheckBoxSizeVal);
		const float IconSize = 16.f;
		const float IconSpacing = 6.f;

		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		float TotalWidth = ImGui::GetContentRegionAvail().x;

		float RightBlockWidth = CheckBoxSize.x +
			(bToolTip ? IconSize + IconSpacing : 0) +
			(Components ? IconSize + IconSpacing : 0);

		ImVec2 Pos = Window->DC.CursorPos;
		ImVec2 LabelPos = Pos;

		ImVec2 CheckBoxPos = ImVec2(Pos.x + TotalWidth - CheckBoxSize.x, Pos.y);
		ImVec2 GearPos = ImVec2(CheckBoxPos.x - IconSpacing - IconSize, Pos.y + (CheckBoxSize.y - IconSize) / 2);
		ImVec2 WarnPos = ImVec2(GearPos.x - IconSpacing - IconSize, Pos.y + (CheckBoxSize.y - IconSize) / 2);

		const ImRect Rect(CheckBoxPos, CheckBoxPos + CheckBoxSize);
		const ImRect Clickable = Rect;

		ImGui::ItemSize(Rect, style.FramePadding.y);

		if (!ImGui::ItemAdd(Rect, id, &Clickable)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(Clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(18, 18, 20);
			ImVec4 LabelColor = g_Col.SecundaryText;

			ImVec2 BackGroundSize = ImVec2(0, 0);
			float CheckUp = 0.f;
			float AnimKeyBind = 0.f;
			float PopupAlpha = 0.f;

			float SlideX = 0.f;
			ImVec4 IconColor = ImColor(245, 158, 66, 0);
			float GearRotation = 0.f;
			bool PopupActive = false;
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto& animRef = anim[id];

		animRef.LabelColor = ImLerp(animRef.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		animRef.BackGroundColor = ImLerp(animRef.BackGroundColor, *Checked ? ImColor(255, 212, 0) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		animRef.UnCheckedBackGroundColor = ImLerp(animRef.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		animRef.BackGroundSize = ImLerp(animRef.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (animRef.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			animRef.CheckColor = ImLerp(animRef.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			animRef.CheckUp = ImLerp(animRef.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			animRef.CheckColor = ImLerp(animRef.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			animRef.CheckUp = ImLerp(animRef.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		Window->DrawList->AddText(LabelPos, ImGui::GetColorU32(animRef.LabelColor), Label);

		float tglCfg = ToggleAnim(id, *Checked);
		DrawToggleSwitch(Window->DrawList, CheckBoxPos + ImVec2(0, (CheckBoxSize.y - 16.f) * 0.5f), tglCfg, Hovered);

		if (bToolTip && g_MenuInfo.IsOpen && ImGui::GetStyle().Alpha > 0.0f) {
			ImColor color = ImColor(245, 158, 66, static_cast<int>(ImGui::GetStyle().Alpha * 255.0f));
			Window->DrawList->AddText(g_Variables.FontAwesomeSolid, IconSize, WarnPos, color, ICON_FA_TRIANGLE_EXCLAMATION);

			if (Hovered && g_MenuInfo.IsOpen)
				ToolTip(Label, ToolTipMsg, ToolTipIcon, true);
		}


		static bool IconHovered = false;
		ImVec2 MousePos = ImGui::GetMousePos();
		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, IconSize, ICON_FA_GEAR);

		ImVec2 GearIconMax = GearPos + IconTextSize;
		IconHovered = MousePos.x > GearPos.x && MousePos.x < GearIconMax.x &&
			MousePos.y > GearPos.y && MousePos.y < GearIconMax.y;

		if (IconHovered && g.IO.MouseClicked[0]) {
			animRef.PopupActive = !animRef.PopupActive;
		}
		else if (!IconHovered && g.IO.MouseClicked[0] && animRef.PopupActive) {
			ImVec2 PopupSize(180, 90);
			ImVec2 PopupMin = GearPos + ImVec2(0, IconSize + 4);
			if (!(MousePos.x > PopupMin.x && MousePos.x < PopupMin.x + PopupSize.x &&
				MousePos.y > PopupMin.y && MousePos.y < PopupMin.y + PopupSize.y)) {
				animRef.PopupActive = false;
			}
		}

		animRef.GearRotation = ImLerp(animRef.GearRotation, animRef.PopupActive ? 1.f : -1.f, g.IO.DeltaTime * 10.f);
		animRef.IconColor = ImLerp(animRef.IconColor, animRef.PopupActive ? ImColor(g_Col.Base) : ImColor(60, 60, 60), g.IO.DeltaTime * 10.f);

		ImRotateStart();
		Window->DrawList->AddText(g_Variables.FontAwesomeSolid, IconSize, GearPos, ImGui::GetColorU32(animRef.IconColor), ICON_FA_GEAR);
		ImRotateEnd(1.57f * animRef.GearRotation);

		animRef.PopupAlpha = ImClamp(animRef.PopupAlpha + (6.f * g.IO.DeltaTime * (animRef.PopupActive && g_MenuInfo.IsOpen ? 1.f : -1.f)), 0.f, 1.f);
		if (animRef.PopupAlpha >= 0.001f) {
			PushStyleVar(ImGuiStyleVar_Alpha, animRef.PopupAlpha);
			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
			PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);
			PushStyleColor(ImGuiCol_Border, GetColorU32(ImVec4(ImColor(20, 20, 22))));
			PushStyleColor(ImGuiCol_PopupBg, GetColorU32(ImVec4(ImColor(14, 14, 16))));

			ImVec2 PopupSize(180, 90);
			ImVec2 PopupMin = GearPos + ImVec2(0, IconSize + 4);
			SetNextWindowSize(PopupSize);
			SetNextWindowPos(PopupMin);

			Begin(Label, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_WithoutScrollClamp | ImGuiWindowFlags_Tooltip);
			{
				PopupSize = ImGui::GetWindowSize();
				Components();
			}

			PopStyleVar(7);
			PopStyleColor(2);
			End();
		}


		IMGUI_TEST_ENGINE_ITEM_INFO(id, Label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}

	inline bool CfgButton(const char* Label, std::function<void()> Components)
	{
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string IdStr = (std::string)Label;
		const ImGuiID id = Window->GetID(IdStr.c_str());

		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, ICON_FA_GEAR); //ICON_FA_ELLIPSIS
		ImVec2 MousePos = ImGui::GetMousePos();

		const ImVec2 Pos = Window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(IconTextSize.x, IconTextSize.y));

		ImGui::ItemSize(Rect, style.FramePadding.y);

		if (!ImGui::ItemAdd(Rect, id)) {
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(Rect, id, &Hovered, &Held);
		if (Pressed) {
			ImGui::MarkItemEdited(id);
		}

		struct CfgBtn_t {
			ImVec4 IconColor = ImColor(245, 158, 66, 0);
			float GearRotation = 0.f;
			float PopupAlpha = 0.f;
			bool PopupActive = false;
		};

		static std::map<ImGuiID, CfgBtn_t> anim;
		auto CfgAnim = anim.find(id);

		if (CfgAnim == anim.end())
		{
			anim.insert({ id, CfgBtn_t() });
			CfgAnim = anim.find(id);
		}

		static ImVec2 PopupSize(180, 90);
		ImVec2 PopupMin = Pos + ImVec2(IconTextSize.x + 4, IconTextSize.y + 4);

		if (Hovered && g.IO.MouseClicked[0]) {
			CfgAnim->second.PopupActive = !CfgAnim->second.PopupActive;
		}
		else if (!Hovered && g.IO.MouseClicked[0] && CfgAnim->second.PopupActive) {
			if (!(MousePos.x > PopupMin.x && MousePos.x < PopupMin.x + PopupSize.x && MousePos.y > PopupMin.y && MousePos.y < PopupMin.y + PopupSize.y)) {
				CfgAnim->second.PopupActive = false;
			}
		}

		CfgAnim->second.GearRotation = ImLerp(CfgAnim->second.GearRotation, CfgAnim->second.PopupActive ? 1.f : -1.f, g.IO.DeltaTime * 10.f);
		CfgAnim->second.IconColor = ImLerp(CfgAnim->second.IconColor, CfgAnim->second.PopupActive ? ImColor(g_Col.Base) : ImColor(60, 60, 60), g.IO.DeltaTime * 10.f);

		ImRotateStart();
		Window->DrawList->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, Pos, ImGui::GetColorU32(CfgAnim->second.IconColor), ICON_FA_GEAR); //ICON_FA_ELLIPSIS
		ImRotateEnd(1.57f * CfgAnim->second.GearRotation);

		CfgAnim->second.PopupAlpha = ImClamp(CfgAnim->second.PopupAlpha + (6.f * g.IO.DeltaTime * (CfgAnim->second.PopupActive && g_MenuInfo.IsOpen ? 1.f : -1.f)), 0.f, 1.f);

		if (CfgAnim->second.PopupAlpha >= 0.001f) {
			PushStyleVar(ImGuiStyleVar_Alpha, CfgAnim->second.PopupAlpha);
			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 10));
			PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
			PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);
			PushStyleColor(ImGuiCol_Border, GetColorU32(ImVec4(ImColor(20, 20, 22))));
			PushStyleColor(ImGuiCol_PopupBg, GetColorU32(ImVec4(ImColor(14, 14, 16))));

			SetNextWindowSize(ImVec2(PopupSize.x, 0));
			SetNextWindowPos(PopupMin);

			Begin(Label, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_WithoutScrollClamp | ImGuiWindowFlags_Tooltip);
			{
				PopupSize = ImGui::GetWindowSize();
				Components();
			}

			PopStyleVar(7);
			PopStyleColor(2);

			End();
		}
	}

	inline bool CheckBoxPage(const char* Label, bool* Checked, std::function<void()> Code, bool bToolTip = false, const char* ToolTipMsg = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = (std::string)Label + std::to_string(reinterpret_cast<uintptr_t>(Checked));
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		float Width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - (Window->ScrollbarY ? 5.f : 0.f);

		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		const ImVec2 CheckBoxSize(22, 22);

		const ImVec2 Pos = Window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(Width, CheckBoxSize.y - 4));
		const ImRect Clickable(Pos, Pos + ImVec2(CheckBoxSize.x + TextSize.x + 8, CheckBoxSize.y));

		ImGui::ItemSize(Rect, style.FramePadding.y);

		if (!ImGui::ItemAdd(Rect, id, &Clickable)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(Clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(18, 18, 20);
			ImVec4 LabelColor = g_Col.SecundaryText;

			ImVec2 BackGroundSize = ImVec2(0, 0);

			float CheckUp = 0.f;
			float AnimKeyBind = 0.f;

			float SlideUp = 0.f;
			ImVec4 IconColor = ImColor(245, 158, 66, 0);
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto CheckBoxAnim = anim.find(id);

		if (CheckBoxAnim == anim.end())
		{
			anim.insert({ id, WidCheckBox_t() });
			CheckBoxAnim = anim.find(id);
		}

		CheckBoxAnim->second.LabelColor = ImLerp(CheckBoxAnim->second.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		CheckBoxAnim->second.BackGroundColor = ImLerp(CheckBoxAnim->second.BackGroundColor, *Checked ? ImColor(g_Col.Base) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		CheckBoxAnim->second.UnCheckedBackGroundColor = ImLerp(CheckBoxAnim->second.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		CheckBoxAnim->second.BackGroundSize = ImLerp(CheckBoxAnim->second.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (CheckBoxAnim->second.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		float tglPage = ToggleAnim(id, *Checked);

		DrawToggleSwitch(Window->DrawList, Rect.Min + ImVec2(0, (CheckBoxSize.y - 16.f) * 0.5f), tglPage, Hovered);





		ImVec2 TextPos = ImVec2(Rect.Min.x + 38.f, Pos.y + Rect.Max.y / 2 - (Pos.y + TextSize.y) / 2);
		Window->DrawList->AddText(TextPos, ImGui::GetColorU32(CheckBoxAnim->second.LabelColor), Label);

		////////////////////////////
		// Link Obj
		////////////////////////////
		static bool IconHovered = false;
		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, ICON_FA_SHARE);
		ImVec2 MousePos = ImGui::GetMousePos();
		ImVec2 MinPos = TextPos + ImVec2(TextSize.x, (CheckBoxSize.y / 2 - TextSize.y / 2) - 4);
		ImVec2 MaxPos = TextPos + ImVec2(TextSize.x + (10 * 2) + IconTextSize.x, (MinPos.y - TextPos.y) + IconTextSize.y + 4);

		if (MousePos.x > MinPos.x && MousePos.x < MaxPos.x && MousePos.y > MinPos.y && MousePos.y < MaxPos.y) {
			IconHovered = true;
		}
		else {
			IconHovered = false;
		}

		if (bToolTip) {
			ToolTip(Label, ToolTipMsg, ICON_FA_SHARE, IconHovered && g_MenuInfo.IsOpen);
		}

		CheckBoxAnim->second.SlideUp = ImLerp(CheckBoxAnim->second.SlideUp, IconHovered ? 2.f : 0.f,
			g.IO.DeltaTime * 10.f);
		CheckBoxAnim->second.IconColor = ImLerp(CheckBoxAnim->second.IconColor, IconHovered
			? ImColor(g_Col.Base) : ImColor(60, 60, 60), g.IO.DeltaTime * 10.f);

		Window->DrawList->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4
			, TextPos + ImVec2(TextSize.x + 10, (CheckBoxSize.y / 2 - TextSize.y / 2) - CheckBoxAnim->second.SlideUp),
			ImGui::GetColorU32(CheckBoxAnim->second.IconColor), ICON_FA_SHARE);

		if (IconHovered && g.IO.MouseClicked[0]) {
			Code();
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}

	inline void TextCentered(const char* text, int m) {

		ImVec2 textSize = ImGui::CalcTextSize(text);
		float posX = (g_MenuInfo.MenuSize.x - textSize.x) * 0.5f;
		float posY = (g_MenuInfo.MenuSize.y - textSize.y) * 0.5f;

		switch (m) {

		case 0:
			ImGui::SetCursorPos({ posX, posY });
			ImGui::Text(text);
			break;
		case 1:
			ImGui::SetCursorPosX(posX);
			ImGui::Text(text);
			break;
		case 2:
			ImGui::SetCursorPosY(posY);
			ImGui::Text(text);
			break;
		default:
			ImGui::SetCursorPos({ posX, posY });
			ImGui::Text(text);
			break;

		}
	}


	inline bool ButtonWithIcon(const char* icon, const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags) {
		struct button_struct {
			ImVec4 BorderCol;
			ImVec4 background;
			ImVec4 LabelColor;
			float UpBackground;
		};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = g_Variables.m_FontNormal->CalcTextSizeA(g_Variables.m_FontNormal->FontSize, FLT_MAX, 0, label);
		const ImVec2 IconTextSize = g_Variables.FontAwesomeSolid->CalcTextSizeA(g_Variables.FontAwesomeSolid->FontSize - 4, FLT_MAX, 0, icon);
		const ImVec2 pos = window->DC.CursorPos;

		static std::map<ImGuiID, button_struct> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end()) {
			anim.insert({ id, button_struct() });
			it_anim = anim.find(id);
		}

		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + IconTextSize.x + style.FramePadding.x * 3.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, 0.f);

		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool Hovered, Held, Pressed = ImGui::ButtonBehavior(bb, id, &Hovered, &Held, flags);

		it_anim->second.BorderCol = ImLerp(it_anim->second.BorderCol, Hovered ? ImVec4(g_Col.Base) : ImVec4(ImColor(18, 18, 20)), g.IO.DeltaTime * 6.f);
		it_anim->second.background = ImLerp(it_anim->second.background, Hovered ? ImVec4(ImColor(14, 14, 16)) : ImVec4(ImColor(14, 14, 16)), g.IO.DeltaTime * 6.f);
		it_anim->second.LabelColor = ImLerp(it_anim->second.LabelColor, Hovered ? ImVec4(ImColor(225, 225, 225)) : ImVec4(ImColor(225, 225, 225)), g.IO.DeltaTime * 6.f);
		it_anim->second.UpBackground = ImLerp(it_anim->second.UpBackground, Hovered ? (bb.Min.y - pos.y) - (bb.Max.y - pos.y) : 0.f, g.IO.DeltaTime * 6.f);

		float totalWidth = label_size.x + IconTextSize.x + style.FramePadding.x;
		ImVec2 TextPos = { pos.x + (size.x - totalWidth) * 0.5f + IconTextSize.x + style.FramePadding.x, pos.y + (size.y - label_size.y) * 0.5f - 0.5f };
		ImVec2 TextWithoutIconPos = { pos.x + (size.x - label_size.x) * 0.5f + style.FramePadding.x, pos.y + (size.y - label_size.y) * 0.5f - 0.5f };
		ImVec2 IconPos = { pos.x + (size.x - totalWidth) * 0.5f, pos.y + (size.y - IconTextSize.y) * 0.5f - 0.5f };

		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(it_anim->second.background), 4);
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(it_anim->second.BorderCol), 4);

		window->DrawList->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y), ImVec2(bb.Max.x, bb.Max.y + it_anim->second.UpBackground), ImGui::GetColorU32(g_Col.Base), 4);

		//window->DrawList->AddText( g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, IconPos, ImGui::GetColorU32( it_anim->second.LabelColor ), icon );
		window->DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, TextWithoutIconPos, ImGui::GetColorU32(it_anim->second.LabelColor), label);

		return Pressed;
	}

	inline bool Button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, bool bToolTip = false, const char* ToolTipMsg = "", const char* ToolTipIcon = "")
	{
		struct button13Anims {
			float closing_anim;
			float closing_alpha;
			float label_alpha;
			bool animation_complete;
		};

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		ImDrawList* draw = GetWindowDrawList();

		static std::map<ImGuiID, button13Anims> anim; // #include <map> on top of this file, where is "imgui.h" located.
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, button13Anims() });
			it_anim = anim.find(id);
		}

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ItemSize(size, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		if (bToolTip) {
			ToolTip(label, ToolTipMsg, ToolTipIcon, hovered && g_MenuInfo.IsOpen);
		}

		it_anim->second.closing_anim = ImLerp(it_anim->second.closing_anim, (hovered ? size.y : 0), g.IO.DeltaTime * 8.f);

		if (hovered || pressed) {
			if (it_anim->second.label_alpha < 255.f)
				it_anim->second.label_alpha += 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha < 255.f)
				it_anim->second.closing_alpha += 15.f / GetIO().Framerate * 160.f;
		}
		else {
			if (it_anim->second.label_alpha > 0.f)
				it_anim->second.label_alpha -= 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha > 0.f)
				it_anim->second.closing_alpha -= 10.f / GetIO().Framerate * 160.f;
		}

		// Render
		const ImU32 inside_solid_col = GetColorU32(ImVec4(ImColor(16, 16, 18)));
		const ImU32 outside_solid_col = GetColorU32(ImVec4(ImColor(24, 24, 26)));
		const ImU32 inside_hover_col = GetColorU32(ImVec4(ImColor(255, 212, 0, (int)it_anim->second.closing_alpha)));

		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y), inside_solid_col, 6);
		draw->AddRect(bb.Min, bb.Max, outside_solid_col, 6);
		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y - it_anim->second.closing_anim), ImVec2(bb.Max.x, bb.Max.y), inside_hover_col, 6);

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(140, 140, 140, 255 - (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(20, 20, 20, (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

		return pressed;
	}

	inline bool ButtonHeld(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
	{
		struct button13Anims {
			float closing_anim;
			float closing_alpha;
			float label_alpha;
			bool animation_complete;
		};

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		ImDrawList* draw = GetWindowDrawList();

		static std::map<ImGuiID, button13Anims> anim; // #include <map> on top of this file, where is "imgui.h" located.
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, button13Anims() });
			it_anim = anim.find(id);
		}

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ItemSize(size, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		it_anim->second.closing_anim = ImLerp(it_anim->second.closing_anim, (held ? size.y : 0), g.IO.DeltaTime * 8.f);

		if (held || pressed) {
			if (it_anim->second.label_alpha < 255.f)
				it_anim->second.label_alpha += 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha < 255.f)
				it_anim->second.closing_alpha += 15.f / GetIO().Framerate * 160.f;
		}
		else {
			if (it_anim->second.label_alpha > 0.f)
				it_anim->second.label_alpha -= 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha > 0.f)
				it_anim->second.closing_alpha -= 10.f / GetIO().Framerate * 160.f;
		}

		// Render
		const ImU32 inside_solid_col = GetColorU32(ImVec4(ImColor(16, 16, 18)));
		const ImU32 outside_solid_col = GetColorU32(ImVec4(ImColor(24, 24, 26)));
		const ImU32 inside_hover_col = ImColor(255, 212, 0, (int)it_anim->second.closing_alpha);

		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y), inside_solid_col, 6);
		draw->AddRect(bb.Min, bb.Max, outside_solid_col, 6);
		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y - it_anim->second.closing_anim), ImVec2(bb.Max.x, bb.Max.y), inside_hover_col, 6);

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(140, 140, 140, 255 - (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(225, 225, 225, (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

		bool completed = it_anim->second.closing_anim >= (size.y - 1.f);
		if (completed && !it_anim->second.animation_complete) {
			it_anim->second.animation_complete = true;
			return true;
		}

		if (pressed) {
			it_anim->second.animation_complete = false;
		}

		return false;
	}

	inline bool WeaponButtonHeld(ImTextureID Icon, const char* label, ImGuiButtonFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImVec2 LabelSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, label);
		const ImGuiID id = window->GetID(label);
		ImDrawList* Draw = window->DrawList;

		struct WeaponButtonHeld_t {
			ImVec4 ProgressCol = ImColor();
			ImVec4 ShadowProgressCol = ImColor();
			ImVec4 SlideCol = ImColor();
			float Alpha = 0.f;
			float SlideX = 0.f;
			float SlideXShadow = 0.f;
			float IconSize = 0.f;
			bool Completed = false;
		};

		static std::map<ImGuiID, WeaponButtonHeld_t> anim;
		auto WeaponButtonAnim = anim.find(id);
		if (WeaponButtonAnim == anim.end())
		{
			anim.insert({ id, WeaponButtonHeld_t() });
			WeaponButtonAnim = anim.find(id);
		}

		const float Width = 100;
		const float Height = Width;
		ImVec2 Pos = window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(Width, Height));
		ItemSize(ImVec2(Width, Height), style.FramePadding.y);
		if (!ItemAdd(Rect, id))
			return false;

		if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;

		bool Hovered, Held;
		bool Pressed = ButtonBehavior(Rect, id, &Hovered, &Held, flags);

		bool Condition = WeaponButtonAnim->second.Completed ? false : Held;

		WeaponButtonAnim->second.Alpha = ImLerp(WeaponButtonAnim->second.Alpha, Condition ? 0.8f : Hovered ? 1.f : 0.6f, g.IO.DeltaTime * 8.f);
		WeaponButtonAnim->second.IconSize = ImLerp(WeaponButtonAnim->second.IconSize, Condition ? 4.f : 0.f, g.IO.DeltaTime * 8.f);
		WeaponButtonAnim->second.SlideX = ImLerp(WeaponButtonAnim->second.SlideX, Condition ? Width : 0.f, g.IO.DeltaTime * 2.2f);
		WeaponButtonAnim->second.SlideXShadow = ImLerp(WeaponButtonAnim->second.SlideXShadow, WeaponButtonAnim->second.SlideX >= 1.f ? WeaponButtonAnim->second.SlideX + 1.f : WeaponButtonAnim->second.SlideX, g.IO.DeltaTime * 3.f);
		WeaponButtonAnim->second.ProgressCol = ImLerp(WeaponButtonAnim->second.ProgressCol, WeaponButtonAnim->second.SlideX >= 0.6f ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 255.f / 255.f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f), g.IO.DeltaTime * 18.f);
		WeaponButtonAnim->second.ShadowProgressCol = ImLerp(WeaponButtonAnim->second.ShadowProgressCol, WeaponButtonAnim->second.SlideXShadow >= 0.6f ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.4f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f), g.IO.DeltaTime * 18.f);

		float Size = (10 + WeaponButtonAnim->second.IconSize);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * WeaponButtonAnim->second.Alpha);
		{
			Draw->AddRectFilled(Rect.Min, Rect.Max, GetColorU32(ImVec4(ImColor(20, 20, 22))), 8);
			//Draw->AddRectFilledMultiColor( Rect.Min, Rect.Max, GetColorU32( ImVec4( ImColor( g_Col.Base ) ), WeaponButtonAnim->second.Alpha ), GetColorU32( ImVec4( ImColor( g_Col.Base ) ), WeaponButtonAnim->second.Alpha ), GetColorU32( ImVec4( ImColor( 20, 20, 22, 0 ) ) ), GetColorU32( ImVec4( ImColor( 20, 20, 22, 0 ) ) ), 8 );
			Draw->AddRectFilled(Pos + ImVec2(0, Height - 4), Pos + ImVec2(WeaponButtonAnim->second.SlideXShadow, Height), GetColorU32(WeaponButtonAnim->second.ShadowProgressCol), 8);
			Draw->AddRectFilled(Pos + ImVec2(0, Height - 4), Pos + ImVec2(WeaponButtonAnim->second.SlideX, Height), GetColorU32(WeaponButtonAnim->second.ProgressCol), 8);
			Draw->AddImage(Icon, ImVec2(Rect.Min.x + Size, Rect.Min.y + Size), ImVec2(Rect.Max.x - Size, Rect.Max.y - Size), { 0,0 }, { 1,1 }, GetColorU32(ImVec4(ImColor(255, 255, 255))));
		}
		ImGui::PopStyleVar();

		Draw->AddRect(Rect.Min - ImVec2(4, 4), Rect.Max + ImVec2(4, 4), GetColorU32(g_Col.BackgroundCol), 8, 0, 8.f);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

		bool Completed = WeaponButtonAnim->second.SlideX >= (Width - 1.f);

		if (WeaponButtonAnim->second.Completed && Held) {
			return false;
		}


		if (Completed) {
			WeaponButtonAnim->second.Completed = true;
			return true;
		}

		if (!Held) {
			WeaponButtonAnim->second.Completed = false;
		}

		return false;
	}

	inline bool ResourceListButton(const char* Label, uint32_t ResourceState, ImVec2 SizeArg, ImGuiButtonFlags flags) {


		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = Window->GetID(Label);
		const ImVec2 LabelSize = g_Variables.m_FontNormal->CalcTextSizeA(g_Variables.m_FontNormal->FontSize, FLT_MAX, 0, Label);
		const ImVec2 TwoTextSize = g_Variables.m_FontSecundary->CalcTextSizeA(g_Variables.m_FontSecundary->FontSize, FLT_MAX, 0, xorstr("Stop"));
		const ImVec2 IconTextSize = g_Variables.FontAwesomeSolid->CalcTextSizeA(g_Variables.FontAwesomeSolid->FontSize, FLT_MAX, 0, ICON_FA_CIRCLE_STOP);
		const ImVec2 pos = Window->DC.CursorPos;


		float Width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - (Window->ScrollbarY ? 12.f : 0.f);
		SizeArg.x = Width;
		ImVec2 size = ImGui::CalcItemSize(SizeArg, LabelSize.x + IconTextSize.x + style.FramePadding.x * 3.0f, LabelSize.y + style.FramePadding.y * 2.0f);

		const ImRect Rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, 0.f);

		if (!ImGui::ItemAdd(Rect, id))
			return false;

		bool Hovered, Held, Pressed = ImGui::ButtonBehavior(Rect, id, &Hovered, &Held, flags);

		struct ResourceBtn_t {
			float Alpha = 0.f;
			float SlideHeld = 0.f;
			float Size = 0.f;
			bool Completed = false;
		};

		static std::map<ImGuiID, ResourceBtn_t> anim;
		auto ResourceBtnAnim = anim.find(id);

		if (ResourceBtnAnim == anim.end()) {
			anim.insert({ id, ResourceBtn_t() });
			ResourceBtnAnim = anim.find(id);
		}

		auto TextPos = ImVec2(Rect.Min.x + 8, pos.y + (size.y / 2 - LabelSize.y / 2));

		ResourceBtnAnim->second.Alpha = ImLerp(ResourceBtnAnim->second.Alpha, ResourceState == 3 ? 1.f : 0.4f, g.IO.DeltaTime * 8);
		ResourceBtnAnim->second.SlideHeld = ImLerp(ResourceBtnAnim->second.SlideHeld, Held && ResourceState == 3 ? Rect.Max.x - pos.x : TextPos.x - pos.x + LabelSize.x + 8, g.IO.DeltaTime * 4);
		ResourceBtnAnim->second.Size = ImLerp(ResourceBtnAnim->second.Size, ResourceState == 3 && Held ? 4.f : 3.f, g.IO.DeltaTime * 4);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ResourceBtnAnim->second.Alpha);

		Window->DrawList->AddRectFilled(Rect.Min, Rect.Max, GetColorU32((ImVec4)ImColor(16, 16, 18)), 6);
		Window->DrawList->AddRectFilled(Rect.Min, ImVec2(pos.x + ResourceBtnAnim->second.SlideHeld, Rect.Max.y), GetColorU32((ImVec4)ImColor(24, 24, 26)), 6);
		Window->DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, TextPos, GetColorU32(ImVec4(ImColor(g_Col.FeaturesText))), Label);

		ImVec2 CirclePos(Rect.Max.x - 16, pos.y + (size.y) / 2);

		if (ResourceState == 3) {
			Window->DrawList->AddCircleFilled(CirclePos, ResourceBtnAnim->second.Size, GetColorU32(ImVec4(ImColor(55, 237, 125, 180))), 99);
		}
		else {
			Window->DrawList->AddCircleFilled(CirclePos, ResourceBtnAnim->second.Size, GetColorU32(ImVec4(ImColor(237, 55, 55, 180))), 99);
		}

		ImGui::PopStyleVar();

		if (ResourceState == 3) {
			if (Held && ResourceBtnAnim->second.SlideHeld >= (Rect.Max.x - pos.x) - 1.f)
			{
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	inline bool ListSelectableCustom(const char* Label, bool* Selected, int Type)
	{
		if (Type == 0) //Players
		{
			//if ( SelectableCustomResources( Label, *Selected, 3 ) ) {
				//*Selected = !*Selected;
				//return true;
			//}
		}
		else if (Type == 1) //Vehicles
		{
			//if ( SelectableCustomResources( Label, *Selected, 3 ) ) {
				//*Selected = !*Selected;
				//return true;
			//}
		}

		return false;
	}

}
