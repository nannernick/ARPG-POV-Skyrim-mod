#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// 2D outline maths for the HUD: the outline of something on screen is the convex hull of
// its projected points, pushed outward a few pixels.
namespace Hull
{
	struct Vec2
	{
		float x;
		float y;
	};

	inline float Cross(const Vec2& a_o, const Vec2& a_a, const Vec2& a_b)
	{
		return (a_a.x - a_o.x) * (a_b.y - a_o.y) - (a_a.y - a_o.y) * (a_b.x - a_o.x);
	}

	// Convex hull (monotone chain). Points that don't span an area give the box
	// around them, so the result is always a usable polygon.
	inline std::vector<Vec2> Convex(std::vector<Vec2> a_points)
	{
		std::sort(a_points.begin(), a_points.end(), [](const Vec2& a_lhs, const Vec2& a_rhs) {
			return a_lhs.x < a_rhs.x || (a_lhs.x == a_rhs.x && a_lhs.y < a_rhs.y);
		});
		std::vector<Vec2> hull;
		if (a_points.empty()) {
			return hull;
		}
		hull.resize(a_points.size() * 2);
		std::size_t k = 0;
		for (std::size_t i = 0; i < a_points.size(); ++i) {
			while (k >= 2 && Cross(hull[k - 2], hull[k - 1], a_points[i]) <= 1e-4f) {
				--k;
			}
			hull[k++] = a_points[i];
		}
		const std::size_t lower = k + 1;
		for (std::size_t i = a_points.size() - 1; i > 0; --i) {
			while (k >= lower && Cross(hull[k - 2], hull[k - 1], a_points[i - 1]) <= 1e-4f) {
				--k;
			}
			hull[k++] = a_points[i - 1];
		}
		hull.resize(k > 1 ? k - 1 : k);

		if (hull.size() < 3) {
			// A single point or a straight line: the box around it.
			float minX = a_points.front().x;
			float maxX = minX;
			float minY = a_points.front().y;
			float maxY = minY;
			for (const auto& p : a_points) {
				minX = std::min(minX, p.x);
				maxX = std::max(maxX, p.x);
				minY = std::min(minY, p.y);
				maxY = std::max(maxY, p.y);
			}
			hull = { { minX - 1.0f, minY - 1.0f }, { maxX + 1.0f, minY - 1.0f }, { maxX + 1.0f, maxY + 1.0f }, { minX - 1.0f, maxY + 1.0f } };
		}
		return hull;
	}

	inline float SignedArea(const std::vector<Vec2>& a_hull)
	{
		float area = 0.0f;
		for (std::size_t i = 0; i < a_hull.size(); ++i) {
			const Vec2& a = a_hull[i];
			const Vec2& b = a_hull[(i + 1) % a_hull.size()];
			area += a.x * b.y - b.x * a.y;
		}
		return area * 0.5f;
	}

	inline float Area(const std::vector<Vec2>& a_hull)
	{
		return std::fabs(SignedArea(a_hull));
	}

	// Pushes every edge of a convex polygon outward by a_pixels.
	inline std::vector<Vec2> Inflate(const std::vector<Vec2>& a_hull, float a_pixels)
	{
		const std::size_t n = a_hull.size();
		if (n < 3) {
			return a_hull;
		}
		const float       sign = SignedArea(a_hull) >= 0.0f ? 1.0f : -1.0f;
		std::vector<Vec2> out;
		out.reserve(n * 2);
		for (std::size_t i = 0; i < n; ++i) {
			const Vec2& prev = a_hull[(i + n - 1) % n];
			const Vec2& cur = a_hull[i];
			const Vec2& next = a_hull[(i + 1) % n];
			// Unit directions of the two edges that meet here, and their outward normals.
			float       e1x = cur.x - prev.x;
			float       e1y = cur.y - prev.y;
			float       e2x = next.x - cur.x;
			float       e2y = next.y - cur.y;
			const float l1 = std::max(std::hypot(e1x, e1y), 1e-6f);
			const float l2 = std::max(std::hypot(e2x, e2y), 1e-6f);
			e1x /= l1;
			e1y /= l1;
			e2x /= l2;
			e2y /= l2;
			const float n1x = e1y * sign;
			const float n1y = -e1x * sign;
			const float n2x = e2y * sign;
			const float n2y = -e2x * sign;
			const float denom = 1.0f + n1x * n2x + n1y * n2y;
			if (denom < 0.6f) {
				// Sharp corner: a square cap instead of a long spike.
				out.push_back({ cur.x + (n1x + e1x) * a_pixels, cur.y + (n1y + e1y) * a_pixels });
				out.push_back({ cur.x + (n2x - e2x) * a_pixels, cur.y + (n2y - e2y) * a_pixels });
			} else {
				out.push_back({ cur.x + (n1x + n2x) * a_pixels / denom, cur.y + (n1y + n2y) * a_pixels / denom });
			}
		}
		return out;
	}

	inline bool Contains(const std::vector<Vec2>& a_hull, float a_x, float a_y)
	{
		const std::size_t n = a_hull.size();
		if (n < 3) {
			return false;
		}
		const Vec2 p{ a_x, a_y };
		bool       positive = false;
		bool       negative = false;
		for (std::size_t i = 0; i < n; ++i) {
			const float c = Cross(a_hull[i], a_hull[(i + 1) % n], p);
			positive = positive || c > 0.0f;
			negative = negative || c < 0.0f;
			if (positive && negative) {
				return false;
			}
		}
		return true;
	}

	inline Vec2 Centre(const std::vector<Vec2>& a_hull)
	{
		Vec2 c{ 0.0f, 0.0f };
		for (const auto& p : a_hull) {
			c.x += p.x;
			c.y += p.y;
		}
		if (!a_hull.empty()) {
			c.x /= static_cast<float>(a_hull.size());
			c.y /= static_cast<float>(a_hull.size());
		}
		return c;
	}
}
