#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/instance_class.hpp>

namespace Game {

struct Instance;

using DestroyedCallback = void(ECS::Manager&, Game::Instance&, ECS::Entity);

inline DestroyedCallback* destroyedCallbacks[static_cast<uint32_t>(InstanceClass::NUM_CLASSES)]
		= {};

void init_destroyed_callbacks();

}

