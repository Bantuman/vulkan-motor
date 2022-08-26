#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/normal_id.hpp>
#include <core/surface_type.hpp>
#include <core/geom_type.hpp>

#include <glm/ext/vector_float3.hpp>
#include <math/vector3.hpp>
#include <math/color3uint8.hpp>
#include <math/transform.hpp>

struct PlayerController
{
	Math::Transform m_transform = Math::Transform(1); 
	Math::Vector3 m_cameraOffset {};
	Math::Vector3 m_velocity {};
	Math::Vector3 m_acceleration {};
	float m_roll = 0;
};

namespace Game{
	void update_player_controls(float deltaTime);
	void update_player(float deltaTime);
}
