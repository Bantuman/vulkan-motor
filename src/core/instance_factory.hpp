#pragma once

#include <core/instance.hpp>

namespace Game::InstanceFactory {

Instance* create(ECS::Manager& ecs, InstanceClass classID, ECS::Entity& outEntity);

}

