#pragma once

#include <vector>

#include <core/memory.hpp>

#include <ecs/ecs_fwd.hpp>

#include <math/matrix4x4.hpp>

namespace Game {

class Rig;

struct RigComponent {
	explicit RigComponent(Memory::SharedPtr<Rig>, ECS::Entity);

	Memory::SharedPtr<Rig> m_rig;
	std::vector<Math::Matrix4x4> m_finalBoneTransforms;
	ECS::Entity m_rigContainer;
};

void update_rigs(ECS::Manager&);

}

