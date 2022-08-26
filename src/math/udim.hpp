#pragma once

#include <cstdint>

#include <math/vector2.hpp>

namespace Math {

struct UDim {
	float scale;
	int32_t offset;
};

struct UDim2 {
	UDim x;
	UDim y;

	Math::Vector2 get_scale() const {
		return Math::Vector2(x.scale, y.scale);
	}

	Math::Vector2 get_offset() const {
		return Math::Vector2(static_cast<float>(x.offset), static_cast<float>(y.offset));
	}
};

}

