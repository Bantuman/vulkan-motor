#pragma once

#include <string_view>

#include <ecs/ecs_fwd.hpp>

#include <core/instance_class.hpp>

class GameRenderer;

namespace Game {

struct Instance;

Instance* get_singleton(ECS::Manager&, InstanceClass, ECS::Entity& outEntity);

void init_ambience(ECS::Manager&, GameRenderer&);

}

