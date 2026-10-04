#pragma once
// ASTRA 2.0 UI kit — BLACK + YELLOW identity.
// Visual layer only: no gameplay logic, no config access here.
// Contrast rule: TEXT IS ALWAYS OPAQUE. Only surfaces carry alpha (dark glass).
#include <Includes/Includes.hpp>

namespace UI
{
	inline bool ReduceMotion = false;
	inline float MotionStep(float speed = 12.f)
	{
		return ReduceMotion ? 1.f : 1.f - expf(-ImGui::GetIO().DeltaTime * speed);
	}

	// ---------- Palette 2.0 (BLACK + YELLOW, no blue/purple/red) ----------
	inline ImVec4 Background()      { return ImVec4(0x07 / 255.f, 0x07 / 255.f, 0x07 / 255.f, 0.98f); } // solid canvas
	inline ImVec4 Sidebar()         { return ImVec4(0x0D / 255.f, 0x0D / 255.f, 0x0D / 255.f, 1.f); }
	inline ImVec4 SidebarSel()       { return ImVec4(1.f, 212.f / 255.f, 0.f, 0.07f); } // yellow, extremely faint
	inline ImVec4 Surface()         { return ImVec4(0x0E / 255.f, 0x0E / 255.f, 0x0E / 255.f, 0.98f); } // ~rgba(14,14,14,.80)
	inline ImVec4 Surface2()        { return ImVec4(0x12 / 255.f, 0x12 / 255.f, 0x12 / 255.f, 1.f); }
	inline ImVec4 SurfaceHover()    { return ImVec4(0x18 / 255.f, 0x18 / 255.f, 0x18 / 255.f, 1.f); }
	inline ImVec4 Border()          { return ImVec4(0x24 / 255.f, 0x24 / 255.f, 0x24 / 255.f, 1.f); }
	inline ImVec4 BorderSoft()      { return ImVec4(0x24 / 255.f, 0x24 / 255.f, 0x24 / 255.f, 0.55f); }
	inline ImVec4 Accent()          { return ImVec4(1.f, 212.f / 255.f, 0.f, 1.f); }          // #FFD400
	inline ImVec4 AccentHover()     { return ImVec4(1.f, 224.f / 255.f, 51.f / 255.f, 1.f); } // #FFE033
	inline ImVec4 AccentDark()      { return ImVec4(0xB9 / 255.f, 0x9A / 255.f, 0.f, 1.f); }  // #B99A00
	inline ImVec4 Text()            { return ImVec4(0xF5 / 255.f, 0xF5 / 255.f, 0xF5 / 255.f, 1.f); } // opaque!
	inline ImVec4 TextDim()         { return ImVec4(0xA6 / 255.f, 0xA6 / 255.f, 0xA6 / 255.f, 1.f); } // opaque!
	inline ImVec4 Disabled()        { return ImVec4(0x4A / 255.f, 0x4A / 255.f, 0x4A / 255.f, 1.f); }
	inline ImVec4 TrackOff()        { return ImVec4(0x29 / 255.f, 0x29 / 255.f, 0x29 / 255.f, 1.f); } // #292929
	inline ImVec4 InkOnAccent()     { return ImVec4(0x0A / 255.f, 0x0A / 255.f, 0x0A / 255.f, 1.f); } // near-black text

	// ---------- Dimensions ----------
	namespace Dim
	{
		constexpr float WinW = 1050.f, WinH = 600.f;
		constexpr float SidebarW = 210.f;
		constexpr float HeaderH = 40.f;
		constexpr float Radius = 12.f;
		constexpr float CardPad = 15.f;
		constexpr float ToggleW = 32.f, ToggleH = 17.f;
		constexpr float SliderH = 3.f;
		constexpr float BtnRadius = 7.f;
	}

	inline ImFont* SafeFont(ImFont* f) { return f ? f : ImGui::GetFont(); }

	// Subpage visibility helper: -1 means "show all sections".
	inline bool SectionVisible(int tab, int section)
	{
		if (tab < 0 || tab >= 8) return true;
		int s = g_MenuInfo.SubPage[tab];
		return (s < 0 || s == section);
	}

	struct Flow
	{
		bool first = true;
		void Next() { if (!first) ImGui::SameLine(); first = false; }
	};

	// ---------- Discrete gold particles (behind content, clipped) ----------
	namespace Particles
	{
		struct Dot { float x, y, vx, vy, r, phase, speed; };
		constexpr int kMax = 36;

		struct State
		{
			bool init = false;
			Dot dots[kMax];
			float fade = 0.f; // gradual appearance for open animation
		};
		inline State& Get() { static State s; return s; }

		inline float Hash01(unsigned n)
		{
			n = (n ^ 61u) ^ (n >> 16u);
			n += (n << 3u); n ^= (n >> 4u);
			n *= 0x27d4eb2du; n ^= (n >> 15u);
			return (n % 10000u) / 10000.f;
		}

		// Must be called once per frame BEFORE content, with panel rect in screen coords.
		inline void DrawBehind(const ImVec2& rMin, const ImVec2& rMax, float dt, float targetFade = 1.f)
		{
			State& s = Get();
			float w = rMax.x - rMin.x, h = rMax.y - rMin.y;
			if (w < 40.f || h < 40.f) return;
			if (!s.init)
			{
				for (int i = 0; i < kMax; ++i)
				{
					Dot& d = s.dots[i];
					d.x = Hash01(i * 3u + 1u) * w;
					d.y = Hash01(i * 7u + 2u) * h;
					d.vx = (Hash01(i * 13u + 3u) - 0.5f) * 6.f;   // px/s, very slow
					d.vy = -(3.f + Hash01(i * 17u + 4u) * 7.f);   // drift upward
					d.r = 1.f + Hash01(i * 29u + 5u) * 1.6f;       // 1.0 - 2.6px
					d.phase = Hash01(i * 31u + 6u) * 6.2831f;
					d.speed = 0.4f + Hash01(i * 37u + 7u) * 0.9f;
				}
				s.init = true;
			}
			s.fade = ImLerp(s.fade, targetFade, ImClamp(dt * 3.f, 0.f, 1.f)); // ~330ms appearance

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->PushClipRect(rMin, rMax, true);
			float t = (float)ImGui::GetTime();
			for (int i = 0; i < kMax; ++i)
			{
				Dot& d = s.dots[i];
				d.x += d.vx * dt;
				d.y += d.vy * dt;
				if (d.y < -4.f) { d.y = h + 4.f; d.x = Hash01((unsigned)(i * 91u + (int)(t))) * w; }
				if (d.x < -4.f) d.x = w + 4.f;
				if (d.x > w + 4.f) d.x = -4.f;
				float tw = 0.55f + 0.45f * sinf(t * d.speed + d.phase); // soft fade
				float a = 0.10f + 0.10f * tw; // 0.10 - 0.20, extremely discreet
				dl->AddCircleFilled(ImVec2(rMin.x + d.x, rMin.y + d.y), d.r,
					ImGui::GetColorU32(ImVec4(1.f, 0.83f, 0.25f, a * s.fade)), 10);
			}
			dl->PopClipRect();
		}
	}

	// ---------- Open/close animation (fade + subtle scale, ~300ms) ----------
	inline float OpenAlpha(float dt, bool visible)
	{
		static float a = 0.f;
		a = ImLerp(a, visible ? 1.f : 0.f, ImClamp(dt * 9.f, 0.f, 1.f)); // ~330ms in, fast out
		return ImClamp(a, 0.f, 1.f);
	}

	// ---------- Sidebar nav item: icon + label, yellow indicator ----------
	inline bool NavItem(const char* id, const char* icon, const char* label, bool selected, float width)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;
		ImGuiContext& g = *GImGui;
		const ImGuiID wid = window->GetID(id);
		const float h = 40.f;
		ImVec2 pos = window->DC.CursorPos;
		const ImRect bb(pos, pos + ImVec2(width, h));
		ImGui::ItemSize(ImVec2(width, h), 0.f);
		if (!ImGui::ItemAdd(bb, wid)) return false;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, wid, &hovered, &held);

		struct Anim { ImVec4 col = ImVec4(0, 0, 0, 0); float bar = 0.f; };
		static std::map<ImGuiID, Anim> anim;
		Anim& a = anim[wid];
		float dt = g.IO.DeltaTime;
		a.col = ImLerp(a.col, selected ? Text() : (hovered ? Text() : TextDim()), ImClamp(dt * 10.f, 0.f, 1.f));
		a.bar = ImLerp(a.bar, selected ? 1.f : 0.f, ImClamp(dt * 10.f, 0.f, 1.f));

		float& fill = *window->StateStorage.GetFloatRef(wid ^ 0x41535452, 0.f);
		fill = ImLerp(fill, selected ? 0.13f : (hovered ? 0.055f : 0.f), MotionStep());
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(1.f, 0.83f, 0.f, fill)), 8.f);

		if (a.bar > 0.01f) // small yellow indicator
		{
			float bh = h * 0.52f * a.bar;
			ImVec2 bc = ImVec2(bb.Min.x + 4.f, bb.Min.y + h * 0.5f);
			window->DrawList->AddLine(ImVec2(bc.x, bc.y - bh * 0.5f), ImVec2(bc.x, bc.y + bh * 0.5f),
				ImGui::GetColorU32(ImVec4(Accent().x, Accent().y, Accent().z, a.bar)), 2.5f);
		}

		ImFont* f = SafeFont(g_Variables.FontAwesomeSolidSmall);
		float fs = f->FontSize;
		ImVec2 isz = f->CalcTextSizeA(fs, FLT_MAX, 0, icon);
		ImU32 icol = ImGui::GetColorU32(selected ? Accent() : ImVec4(a.col.x, a.col.y, a.col.z, 1.f));
		window->DrawList->AddText(f, fs, ImVec2(bb.Min.x + 16.f, bb.Min.y + (h - isz.y) * 0.5f), icol, icon);
		window->DrawList->AddText(ImVec2(bb.Min.x + 40.f, bb.Min.y + (h - ImGui::CalcTextSize(label).y) * 0.5f),
			ImGui::GetColorU32(a.col), label);
		return pressed;
	}

	// Horizontal section tabs have their own visual hierarchy.
	inline bool SectionTab(const char* label, bool selected, float width)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, selected ? Accent() : Surface2());
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? AccentHover() : SurfaceHover());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, AccentDark());
		ImGui::PushStyleColor(ImGuiCol_Text, selected ? InkOnAccent() : TextDim());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
		const bool pressed = ImGui::Button(label, ImVec2(width, 34.f));
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);
		return pressed;
	}

	// ---------- Legacy icon-only rail button (kept for compat), now yellow ----------
	inline bool NavIcon(const char* id, const char* icon, bool selected, const ImVec2& size, const char* tooltip = nullptr)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;
		ImGuiContext& g = *GImGui;
		const ImGuiID wid = window->GetID(id);
		ImVec2 pos = window->DC.CursorPos;
		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(size, 0.f);
		if (!ImGui::ItemAdd(bb, wid)) return false;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, wid, &hovered, &held);

		struct NavAnim { ImVec4 col = ImVec4(0, 0, 0, 0); float bar = 0.f; };
		static std::map<ImGuiID, NavAnim> anim;
		NavAnim& a = anim[wid];
		float dt = g.IO.DeltaTime;
		a.col = ImLerp(a.col, selected ? Accent() : (hovered ? Text() : TextDim()), ImClamp(dt * 10.f, 0.f, 1.f));
		a.bar = ImLerp(a.bar, selected ? 1.f : 0.f, ImClamp(dt * 10.f, 0.f, 1.f));

		if (hovered && !selected)
			window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.04f)), 6.f);
		if (selected)
			window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(SidebarSel()), 6.f);
		if (a.bar > 0.01f)
		{
			float bh = size.y * 0.55f * a.bar;
			ImVec2 bc = ImVec2(bb.Min.x + 3.f, bb.Min.y + size.y * 0.5f);
			window->DrawList->AddLine(ImVec2(bc.x, bc.y - bh * 0.5f), ImVec2(bc.x, bc.y + bh * 0.5f),
				ImGui::GetColorU32(ImVec4(Accent().x, Accent().y, Accent().z, a.bar)), 2.f);
		}
		ImFont* f = SafeFont(g_Variables.FontAwesomeSolid);
		float fs = f->FontSize;
		ImVec2 isz = f->CalcTextSizeA(fs, FLT_MAX, 0, icon);
		window->DrawList->AddText(f, fs, bb.Min + (size - isz) * 0.5f, ImGui::GetColorU32(a.col), icon);
		if (hovered && tooltip && tooltip[0])
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.f);
			ImGui::PushStyleColor(ImGuiCol_PopupBg, Surface2());
			ImGui::PushStyleColor(ImGuiCol_Border, Border());
			ImGui::PushStyleColor(ImGuiCol_Text, Text());
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(tooltip);
			ImGui::EndTooltip();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
		}
		return pressed;
	}

	inline void Spinner(const char* id, float radius, float thickness, const ImVec4& col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return;
		ImVec2 pos = window->DC.CursorPos;
		ImGui::Dummy(ImVec2(radius * 2.f, radius * 2.f));
		ImVec2 c = pos + ImVec2(radius, radius);
		float t = (float)ImGui::GetTime();
		const int segs = 24;
		window->DrawList->PathClear();
		for (int i = 0; i < segs; ++i)
		{
			float a0 = (i / (float)segs) * IM_PI * 2.f + t * 4.f;
			float a1 = ((i + 1) / (float)segs) * IM_PI * 2.f + t * 4.f;
			float alpha = (i / (float)segs);
			window->DrawList->PathLineTo(c + ImVec2(cosf(a0), sinf(a0)) * radius);
			window->DrawList->PathLineTo(c + ImVec2(cosf(a1), sinf(a1)) * radius);
			window->DrawList->PathStroke(ImGui::GetColorU32(ImVec4(col.x, col.y, col.z, alpha)), 0, thickness);
		}
		(void)id;
	}

	// ---------- Secondary tree item (FA icon, yellow when selected) ----------
	inline bool TreeItem(const char* id, const char* icon, const char* label, bool selected, float width)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;
		ImGuiContext& g = *GImGui;
		const ImGuiID wid = window->GetID(id);
		const float h = 28.f;
		ImVec2 pos = window->DC.CursorPos;
		const ImRect bb(pos, pos + ImVec2(width, h));
		ImGui::ItemSize(ImVec2(width, h), 0.f);
		if (!ImGui::ItemAdd(bb, wid)) return false;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, wid, &hovered, &held);

		static std::map<ImGuiID, ImVec4> anim;
		ImVec4& c = anim[wid];
		c = ImLerp(c, selected ? Text() : (hovered ? Text() : TextDim()), ImClamp(g.IO.DeltaTime * 10.f, 0.f, 1.f));

		if (selected)
			window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(SidebarSel()), 5.f);
		else if (hovered)
			window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.03f)), 5.f);
		if (selected)
			window->DrawList->AddCircleFilled(ImVec2(bb.Min.x + 5.f, bb.Min.y + h * 0.5f),
				2.5f, ImGui::GetColorU32(Accent()), 16);

		ImFont* f = SafeFont(g_Variables.FontAwesomeSolidSmall);
		float fs = f->FontSize;
		const char* glyph = (icon && icon[0]) ? icon : (selected ? "\xE2\x97\x86" : "\xE2\x97\x87");
		ImVec2 gsz = f->CalcTextSizeA(fs, FLT_MAX, 0, glyph);
		ImU32 acc = ImGui::GetColorU32(ImVec4(Accent().x, Accent().y, Accent().z, selected ? 1.f : (hovered ? 0.8f : 0.45f)));
		window->DrawList->AddText(f, fs, ImVec2(bb.Min.x + 14.f, bb.Min.y + (h - gsz.y) * 0.5f), acc, glyph);
		window->DrawList->AddText(ImVec2(bb.Min.x + 36.f, bb.Min.y + (h - ImGui::CalcTextSize(label).y) * 0.5f),
			ImGui::GetColorU32(c), label);
		return pressed;
	}

	// ---------- Toggle switch 2.0: graphite OFF, yellow ON (same bool* contract) ----------
	inline bool Toggle(const char* label, bool* v, const char* id_suffix = nullptr)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems || !window->Active || window->Hidden) return false;
		ImGuiContext& g = *GImGui;
		std::string uid = std::string(label) + (id_suffix ? id_suffix : "") + std::to_string((uintptr_t)v);
		const ImGuiID wid = window->GetID(uid.c_str());

		ImVec2 label_size = ImGui::CalcTextSize(label);
		const float tw = Dim::ToggleW, th = Dim::ToggleH;
		float avail = ImGui::GetContentRegionAvail().x;
		float total_w = (avail > 0.f ? avail : (label_size.x + 8.f + tw));
		const float h = ImMax(label_size.y, th);
		ImVec2 pos = window->DC.CursorPos;
		const ImRect bb(pos, pos + ImVec2(total_w, h));
		ImGui::ItemSize(ImVec2(total_w, h + 4.f), 0.f);
		if (!ImGui::ItemAdd(bb, wid)) return false;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, wid, &hovered, &held);
		if (pressed) { *v = !(*v); ImGui::MarkItemEdited(wid); }

		static std::map<ImGuiID, float> anim;
		float& t = anim[wid];
		t = ImLerp(t, *v ? 1.f : 0.f, ImClamp(g.IO.DeltaTime * 12.f, 0.f, 1.f));

		ImVec2 track_min = ImVec2(bb.Max.x - tw, bb.Min.y + (h - th) * 0.5f);
		float r = th * 0.5f;
		ImVec4 off = hovered ? ImVec4(0x33 / 255.f, 0x33 / 255.f, 0x33 / 255.f, 1.f) : TrackOff();
		ImVec4 on = hovered ? AccentHover() : Accent();
		ImVec4 track = ImLerp(off, on, t);
		window->DrawList->AddRectFilled(track_min, track_min + ImVec2(tw, th), ImGui::GetColorU32(track), r);
		float knob_r = (th - 4.f) * 0.5f;
		float knob_x = ImLerp(track_min.x + 2.f + knob_r, track_min.x + tw - 2.f - knob_r, t);
		ImVec4 knob = ImLerp(ImVec4(0.96f, 0.96f, 0.96f, 1.f), InkOnAccent(), t); // near-white OFF, dark ON
		window->DrawList->AddCircleFilled(ImVec2(knob_x, track_min.y + th * 0.5f), knob_r,
			ImGui::GetColorU32(knob), 20);

		ImVec4 lc = *v ? Text() : TextDim();
		window->DrawList->AddText(ImVec2(bb.Min.x, bb.Min.y + (h - label_size.y) * 0.5f),
			ImGui::GetColorU32(lc), label);
		return pressed;
	}

	// ---------- Thin slider row: label left, value right ----------
	template<typename FN>
	inline void SliderRow(const char* label, const char* value_text, FN&& draw_track)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImVec2 label_size = ImGui::CalcTextSize(label);
		ImVec2 value_size = ImGui::CalcTextSize(value_text);
		float w = ImGui::GetContentRegionAvail().x;
		if (w <= 0.f) w = 200.f;
		window->DrawList->AddText(window->DC.CursorPos, ImGui::GetColorU32(TextDim()), label);
		window->DrawList->AddText(window->DC.CursorPos + ImVec2(w - value_size.x, 0), ImGui::GetColorU32(Accent()), value_text);
		ImGui::Dummy(ImVec2(w, label_size.y + 5.f));
		draw_track(w);
		ImGui::Dummy(ImVec2(w, 5.f));
	}

	inline void ComboRow(const char* label)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		float w = ImGui::GetContentRegionAvail().x;
		if (w <= 0.f) w = 200.f;
		window->DrawList->AddText(window->DC.CursorPos, ImGui::GetColorU32(TextDim()), label);
		ImGui::Dummy(ImVec2(w, ImGui::CalcTextSize(label).y + 2.f));
	}

	// ---------- Card 2.0: translucent, soft border, radius 9 ----------
	inline bool BeginCard(const char* title, const char* subtitle, const ImVec2& size)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Dim::Radius);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Dim::CardPad, 16.f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, Surface());
		ImGui::PushStyleColor(ImGuiCol_Border, BorderSoft());
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Disabled());
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Accent());
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Accent());
		bool ret = ImGui::BeginChild(title, size, true,
			ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
		if (subtitle && subtitle[0])
		{
			ImFont* sf = SafeFont(g_Variables.m_FontSecundary);
			ImGui::PushFont(sf);
			ImGui::TextColored(Text(), "%s", title);
			ImGui::PopFont();
			ImGui::TextColored(TextDim(), "%s", subtitle);
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Separator, BorderSoft());
			ImGui::Separator();
			ImGui::PopStyleColor();
			ImGui::Spacing();
		}
		else
		{
			ImFont* sf = SafeFont(g_Variables.m_FontSecundary);
			ImGui::PushFont(sf);
			ImGui::TextColored(Text(), "%s", title);
			ImGui::PopFont();
			ImGui::Spacing();
		}
		return ret;
	}

	inline void EndCard()
	{
		ImGui::EndChild();
		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(3);
		ImGui::Spacing();
	}

	// ---------- Thin header: category / page + ASTRA build status ----------
	inline void Header(const char* category, const char* page, const char* version)
	{
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const float w = ImMax(50.f, ImGui::GetContentRegionAvail().x - 14.f);
        const ImVec2 pos = window->DC.CursorPos;
        ImDrawList* draw = window->DrawList;
        draw->AddText(pos, ImGui::GetColorU32(Accent()), "PAINEL DE CONTROLE");
        ImFont* titleFont = SafeFont(g_Variables.m_FontSecundary);
        draw->AddText(titleFont, 28.f, pos + ImVec2(0, 24), ImGui::GetColorU32(Text()), category ? category : "");
        draw->AddText(pos + ImVec2(0, 59), ImGui::GetColorU32(TextDim()),
            "Selecione uma secao para ajustar suas preferencias.");
        const char* badge = version ? version : "";
        const ImVec2 badgeSize = ImGui::CalcTextSize(badge);
        const ImVec2 badgePos = pos + ImVec2(w - badgeSize.x - 24.f, 24.f);
        draw->AddRectFilled(badgePos, badgePos + ImVec2(badgeSize.x + 24.f, 28.f), ImGui::GetColorU32(Surface2()), 7.f);
        draw->AddRect(badgePos, badgePos + ImVec2(badgeSize.x + 24.f, 28.f), ImGui::GetColorU32(Border()), 7.f);
        draw->AddText(badgePos + ImVec2(12, (28.f - badgeSize.y) * 0.5f), ImGui::GetColorU32(TextDim()), badge);
        ImGui::Dummy(ImVec2(w, 86.f));
	}

	// ---------- Buttons 2.0: primary yellow / secondary dark ----------
	inline bool PrimaryButton(const char* label, const ImVec2& size)
	{
		const ImGuiID id = ImGui::GetID(label);
		static std::map<ImGuiID, float> hover;
		float& amount = hover[id];
		const ImVec4 color = ImLerp(Accent(), AccentHover(), amount);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, AccentDark());
		ImGui::PushStyleColor(ImGuiCol_Text, InkOnAccent());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Dim::BtnRadius);
		bool r = ImGui::Button(label, size);
		amount = ImLerp(amount, ImGui::IsItemHovered() ? 1.f : 0.f, MotionStep());
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(4);
		return r;
	}

	inline bool SecondaryButton(const char* label, const ImVec2& size)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, Surface2());
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SurfaceHover());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Surface2());
		ImGui::PushStyleColor(ImGuiCol_Text, Text());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Dim::BtnRadius);
		bool r = ImGui::Button(label, size);
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(4);
		return r;
	}

	// ---------- Input styling: translucent bg, yellow focus ring ----------
	inline void PushInputStyle(bool focused)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, Surface());
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SurfaceHover());
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Surface());
		ImGui::PushStyleColor(ImGuiCol_Border, focused ? Accent() : Border());
		ImGui::PushStyleColor(ImGuiCol_Text, Text());
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, TextDim());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
	}
	inline void PopInputStyle()
	{
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(6);
	}
}
