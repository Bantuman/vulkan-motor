#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/instance_class.hpp>

namespace Game {

struct Instance;

// (ecs, selfInst, selfEntity, childInst, childEntity, parentInst, parentEntity)
using AncestryChangedCallback = void(ECS::Manager&, Game::Instance&, ECS::Entity, Game::Instance*,
		ECS::Entity);

inline AncestryChangedCallback*
		ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::NUM_CLASSES)] = {};

void init_ancestry_changed_callbacks();

}

