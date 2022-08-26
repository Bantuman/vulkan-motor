#pragma once

#include <core/memory.hpp>

#include <ecs/ecs_fwd.hpp>

namespace Game {

class Animation;

struct Animator {
	static void create(ECS::Manager&, ECS::Entity);

	Memory::SharedPtr<Animation> m_currentAnim;
	float m_animTime;
};

void update_animators(ECS::Manager&, float deltaTime);

}

