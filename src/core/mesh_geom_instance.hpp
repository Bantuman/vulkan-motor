#pragma once

#include <math/vector4.hpp>
#include <math/transform.hpp>

namespace Game {

struct MeshGeomInstance {
	Math::Transform m_transform;
	Math::Vector4 m_color;
	float m_reflectance;
	struct {
		uint32_t diffuseTexture;
		uint32_t normalTexture;
		uint32_t rig;
	} m_indices;
};

}

