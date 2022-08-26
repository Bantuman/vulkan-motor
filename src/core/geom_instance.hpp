#pragma once

#include <math/vector4.hpp>
#include <math/transform.hpp>

namespace Game {

struct GeomInstance {
	Math::Transform m_transform;
	Math::Vector4 m_scale;
	Math::Vector4 m_color;
	uint32_t m_surfaceTypes;
};

}
