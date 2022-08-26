#pragma once

#include <ecs/ecs_fwd.hpp>

#include <string>

#include <math/vector2.hpp>
#include <math/vector4.hpp>
#include <math/color3.hpp>

#include <core/data_model.hpp>
#include <game/gameworld.hpp>
#include <core/ambience.hpp>

#include <core/sky.hpp>

#include <core/camera.hpp>

#include <core/geom.hpp>
#include <core/imageplane.hpp>
#include <core/mesh_geom.hpp>
#include <animation/animator.hpp>
#include <animation/attachment.hpp>
#include <animation/bone_attachment.hpp>
#include <animation/animator.hpp>
#include <core/model.hpp>

namespace Game {

enum class MeshType : uint8_t {
	HEAD = 0,
	TORSO = 1,
	WEDGE = 2,
	SPHERE = 3,
	CYLINDER = 4,
	FILE_MESH = 5,
	BRICK = 6,
	PRISM = 7,
	PYRAMID = 8,
	PARALLEL_RAMP = 9,
	RIGHT_ANGLE_RAMP = 10,
	CORNER_WEDGE = 11
};

struct JointInstance {
	ECS::Entity m_geom0 = ECS::INVALID_ENTITY;
	ECS::Entity m_geom1 = ECS::INVALID_ENTITY;
	Math::Transform m_c0;
	Math::Transform m_c1;
};

struct SpecialMesh {
	MeshType m_meshType;
	std::string m_meshID;
	std::string m_textureID;
	Math::Vector3 m_offset;
	Math::Vector3 m_scale;
	Math::Vector3 m_vertexColor;
};

}

