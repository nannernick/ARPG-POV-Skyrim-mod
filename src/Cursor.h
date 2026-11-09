#pragma once

#include <cstdint>

// The cursor icons, drawn from simple shapes so no texture files are needed.
// The sword, bag and exclamation mark can have a ring behind them.
// Painter is anything with Line / Tri / Quad / Circle / Arc (the HUD's draw list).
// The click point is always (a_x, a_y): the arrow's tip, the sword's tip, or the small
// pointer next to the bag and the exclamation mark.
namespace Cursor
{
	enum class Icon
	{
		kArrow,  // move
		kSword,  // attack
		kBag,    // loot
		kMark    // talk / enter: exclamation mark
	};

	struct P
	{
		float x;
		float y;
	};

	constexpr std::uint32_t kBlack = 0xFF000000;  // 0xAABBGGRR
	constexpr float         kSymbolOffset = 0.25f; // how far the bag and exclamation mark sit from the pointer (cursor sizes)

	// Each shape is drawn twice: first as a black border, then filled in colour on top.
	template <class Painter>
	struct Pen
	{
		Painter&      painter;
		std::uint32_t color;
		float         border;
		bool          outlinePass;

		void Tri(P a_a, P a_b, P a_c) const
		{
			if (outlinePass) {
				painter.Line(a_a, a_b, kBlack, border * 2.0f);
				painter.Line(a_b, a_c, kBlack, border * 2.0f);
				painter.Line(a_c, a_a, kBlack, border * 2.0f);
			} else {
				painter.Tri(a_a, a_b, a_c, color);
			}
		}

		void Quad(P a_a, P a_b, P a_c, P a_d) const
		{
			if (outlinePass) {
				painter.Line(a_a, a_b, kBlack, border * 2.0f);
				painter.Line(a_b, a_c, kBlack, border * 2.0f);
				painter.Line(a_c, a_d, kBlack, border * 2.0f);
				painter.Line(a_d, a_a, kBlack, border * 2.0f);
			} else {
				painter.Quad(a_a, a_b, a_c, a_d, color);
			}
		}

		void Circle(P a_centre, float a_radius) const
		{
			if (outlinePass) {
				painter.Circle(a_centre, a_radius + border, kBlack);
			} else {
				painter.Circle(a_centre, a_radius, color);
			}
		}
	};

	template <class Painter>
	void Arrow(const Pen<Painter>& a_pen, float a_x, float a_y, float a_s)
	{
		a_pen.Tri({ a_x, a_y }, { a_x, a_y + a_s }, { a_x + a_s * 0.7f, a_y + a_s * 0.7f });
	}

	// Tip at the click point, handle down and to the right.
	template <class Painter>
	void Sword(const Pen<Painter>& a_pen, float a_x, float a_y, float a_s)
	{
		constexpr float k = 0.70710678f;
		// along = distance down the sword from its tip, across = distance to the side (in cursor sizes)
		const auto at = [&](float a_along, float a_across) {
			return P{ a_x + (a_along + a_across) * k * a_s, a_y + (a_along - a_across) * k * a_s };
		};
		a_pen.Tri(at(0.0f, 0.0f), at(0.36f, -0.17f), at(0.36f, 0.17f));                            // point
		a_pen.Quad(at(0.36f, -0.17f), at(0.36f, 0.17f), at(1.1f, 0.17f), at(1.1f, -0.17f));        // blade
		a_pen.Quad(at(1.1f, -0.46f), at(1.1f, 0.46f), at(1.28f, 0.46f), at(1.28f, -0.46f));        // crossguard
		a_pen.Quad(at(1.28f, -0.1f), at(1.28f, 0.1f), at(1.58f, 0.1f), at(1.58f, -0.1f));          // grip
		a_pen.Circle(at(1.66f, 0.0f), 0.15f * a_s);                                                // pommel
	}

	// A filled circle with its own black border, drawn straight away (so overlapping ones keep their edges).
	template <class Painter>
	void Disc(const Pen<Painter>& a_pen, float a_x, float a_y, float a_radius, std::uint32_t a_color)
	{
		if (!a_pen.outlinePass) {
			a_pen.painter.Circle({ a_x, a_y }, a_radius + a_pen.border * 0.8f, kBlack);
			a_pen.painter.Circle({ a_x, a_y }, a_radius, a_color);
		}
	}

	// A sack tipped toward you with its mouth open and coins spilling out, with a small
	// pointer at the click point. The coins take the cursor colour.
	template <class Painter>
	void Bag(const Pen<Painter>& a_pen, float a_x, float a_y, float a_s)
	{
		constexpr std::uint32_t sack = 0xFF4A7DB4;   // tan
		constexpr std::uint32_t rim = 0xFF6FA3D6;    // lighter tan
		constexpr std::uint32_t inside = 0xFF142030; // dark brown
		Arrow(a_pen, a_x, a_y, a_s * 0.5f);
		a_x += kSymbolOffset * a_s;  // the symbol sits clear of the pointer, so its ring doesn't cross it
		a_y += kSymbolOffset * a_s;
		const auto disc = [&](float a_dx, float a_dy, float a_r, std::uint32_t a_color) {
			Disc(a_pen, a_x + a_dx * a_s, a_y + a_dy * a_s, a_r * a_s, a_color);
		};
		disc(1.45f, 0.95f, 0.55f, sack);     // the sack, lying back
		disc(1.02f, 1.32f, 0.4f, rim);       // the open mouth, facing front
		disc(1.02f, 1.32f, 0.3f, inside);
		disc(1.1f, 1.28f, 0.15f, a_pen.color);   // coins inside...
		disc(0.92f, 1.42f, 0.15f, a_pen.color);
		disc(0.62f, 1.72f, 0.15f, a_pen.color);  // ...and spilling out
		disc(0.95f, 1.86f, 0.15f, a_pen.color);
		disc(0.4f, 2.0f, 0.15f, a_pen.color);
		disc(0.74f, 2.12f, 0.15f, a_pen.color);
		disc(1.3f, 2.02f, 0.15f, a_pen.color);
	}

	// A big exclamation mark, with a small pointer at the click point.
	template <class Painter>
	void Mark(const Pen<Painter>& a_pen, float a_x, float a_y, float a_s)
	{
		Arrow(a_pen, a_x, a_y, a_s * 0.5f);
		a_x += kSymbolOffset * a_s;
		a_y += kSymbolOffset * a_s;
		const float cx = a_x + a_s * 1.1f;
		a_pen.Quad({ cx - 0.3f * a_s, a_y + 0.35f * a_s }, { cx + 0.3f * a_s, a_y + 0.35f * a_s }, { cx + 0.15f * a_s, a_y + 1.75f * a_s },
			{ cx - 0.15f * a_s, a_y + 1.75f * a_s });
		a_pen.Circle({ cx, a_y + 2.2f * a_s }, 0.24f * a_s);
	}

	// a_ring: a ring in the symbol's colour, as wide as the symbol is long, so the symbol's
	// two ends lie across it. The ring stops short of the symbol at each end, which makes it
	// read as sitting behind.
	template <class Painter>
	void Draw(Painter& a_painter, Icon a_icon, float a_x, float a_y, float a_size, std::uint32_t a_color, bool a_ring)
	{
		if (a_ring && a_icon != Icon::kArrow) {
			constexpr float deg = 0.0174532925f;
			// Ring centre and radius in cursor sizes, and where the symbol's two ends cross it:
			// the angle of each crossing (0 = right, 90 = down) and how much ring to leave out either side.
			float cx = 0.64f;  // sword: tip (up-left) to pommel (down-right)
			float cy = 0.64f;
			float r = 0.905f;
			float gapA = 45.0f;
			float halfA = 21.0f;
			float gapB = 225.0f;
			float halfB = 21.0f;
			if (a_icon == Icon::kBag) {  // sack (up-right) to the furthest coin (down-left)
				cx = 1.066f + kSymbolOffset;
				cy = 1.334f + kSymbolOffset;
				r = 1.09f;
				gapA = -45.0f;
				halfA = 32.0f;
				gapB = 135.0f;
				halfB = 40.0f;
			} else if (a_icon == Icon::kMark) {  // top of the bar to the bottom of the dot
				cx = 1.1f + kSymbolOffset;
				cy = 1.395f + kSymbolOffset;
				r = 1.045f;
				gapA = -90.0f;
				halfA = 29.0f;
				gapB = 90.0f;
				halfB = 25.0f;
			}
			const P     centre{ a_x + cx * a_size, a_y + cy * a_size };
			const float radius = r * a_size;
			const float thickness = a_size * 0.1f > 1.5f ? a_size * 0.1f : 1.5f;
			const float cap = 1.2f / radius;  // the black border wraps round the ends of each arc
			const float from1 = (gapA + halfA) * deg;
			const float to1 = (gapB - halfB) * deg;
			const float from2 = (gapB + halfB) * deg;
			const float to2 = (gapA + 360.0f - halfA) * deg;
			a_painter.Arc(centre, radius, from1 - cap, to1 + cap, kBlack, thickness + 2.4f);
			a_painter.Arc(centre, radius, from2 - cap, to2 + cap, kBlack, thickness + 2.4f);
			a_painter.Arc(centre, radius, from1, to1, a_color, thickness);
			a_painter.Arc(centre, radius, from2, to2, a_color, thickness);
		}
		for (const bool outlinePass : { true, false }) {
			const Pen<Painter> pen{ a_painter, a_color, 1.5f, outlinePass };
			switch (a_icon) {
			case Icon::kSword:
				Sword(pen, a_x, a_y, a_size);
				break;
			case Icon::kBag:
				Bag(pen, a_x, a_y, a_size);
				break;
			case Icon::kMark:
				Mark(pen, a_x, a_y, a_size);
				break;
			default:
				Arrow(pen, a_x, a_y, a_size);
				break;
			}
		}
	}
}
