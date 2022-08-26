#pragma once

#include <cstdint>

namespace RenderUtils {

constexpr uint32_t previous_power_of_2(uint32_t value) {
	uint32_t r = 1;

	while (2 * r < value) {
		r *= 2;
	}

	return r;
}

constexpr uint32_t get_image_mip_levels(uint32_t width, uint32_t height) {
	uint32_t result = 1;

	while (width > 1 || height > 1) {
		++result;
		width >>= 1;
		height >>= 1;
	}

	return result;
}

constexpr uint32_t get_group_count(uint32_t threadCount, uint32_t localSize) {
	return (threadCount + localSize - 1) / localSize;
}

}

