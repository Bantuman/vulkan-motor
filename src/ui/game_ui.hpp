#pragma once

#include <ecs/ecs.hpp>
#include <ecs/ecs_fwd.hpp>

namespace Game {
	void init_ui(ECS::Manager&);
	void update_ui(ECS::Manager&, float);
}

