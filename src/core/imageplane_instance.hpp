#pragma once

#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/transform.hpp>

namespace Game {

struct DecalInstance {
	Math::Transform m_transform;
	Math::Vector3 m_scale;
	Math::Vector4 m_color;
	uint32_t m_imageIndex;
};

}

