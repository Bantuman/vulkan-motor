#pragma once

#include <cstdint>

namespace Game {

enum class GeomType : uint8_t {
	BALL = 0,
	BLOCK = 1,
	CYLINDER = 2,
	WEDGE = 3,
	CORNER_WEDGE = 4,

	NUM_TYPES
};

}

