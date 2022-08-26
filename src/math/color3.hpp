#pragma once

#include <cstdint>

#include <math/vector4.hpp>
#include <math/color3uint8.hpp>

namespace Math {

struct Color3 {
	static constexpr Color3 from_rgb(float r, float g, float b) {
		return Color3(r / 255.f, g / 255.f, b / 255.f);
	}

	constexpr Color3()
			: r(0.f)
			, g(0.f)
			, b(0.f) {}

	constexpr Color3(float rIn, float gIn, float bIn)
			: r(rIn)
			, g(gIn)
			, b(bIn) {}

	constexpr Color3(uint32_t argb)
			: r(static_cast<float>((argb >> 16) & 0xFF) / 255.f)
			, g(static_cast<float>((argb >> 8) & 0xFF) / 255.f)
			, b(static_cast<float>(argb & 0xFF) / 255.f) {}

	constexpr Color3(Color3uint8 c3u8)
			: r(static_cast<float>((c3u8.argb >> 16) & 0xFF) / 255.f)
			, g(static_cast<float>((c3u8.argb >> 8) & 0xFF) / 255.f)
			, b(static_cast<float>((c3u8.argb & 0xFF) / 255.f)) {}

	Math::Vector4 to_vector4(float alpha) const {
		return Math::Vector4(r, g, b, alpha);
	}

	constexpr bool operator==(const Color3& other) const {
		// FIXME: maybe epsilon comparison
		return r == other.r && g == other.g && b == other.b;
	}

	float r;
	float g;
	float b;
};

}

constexpr Math::Color3 operator*(const Math::Color3& color, float s) {
	return Math::Color3(color.r * s, color.g * s, color.b * s);
}

constexpr Math::Color3 operator*(float s, const Math::Color3& color) {
	return color * s;
}

