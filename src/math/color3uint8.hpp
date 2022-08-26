#pragma once

#include <cstdint>

namespace Math {

struct Color3uint8 {
	explicit Color3uint8() = default;
	explicit Color3uint8(uint32_t argbIn)
			: argb(argbIn) {}
	explicit Color3uint8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF)
			: argb(b | (g << 8) | (r << 16) | (a << 24)) {}

	uint32_t argb;

	constexpr bool operator==(Color3uint8 other) const {
		return argb == other.argb;
	}
};

}

