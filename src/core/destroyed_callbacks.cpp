#include "destroyed_callbacks.hpp"

#include <core/geom.hpp>
#include <core/sky.hpp>

using namespace Game;

void Game::init_destroyed_callbacks() {
	Game::destroyedCallbacks[static_cast<uint32_t>(InstanceClass::CUBE_GEOM)] = Geometry::on_destroyed;
	Game::destroyedCallbacks[static_cast<uint32_t>(InstanceClass::SLOPE_GEOM)]
			= Geometry::on_destroyed;
	Game::destroyedCallbacks[static_cast<uint32_t>(InstanceClass::CORNER_SLOPE_GEOM)]
			= Geometry::on_destroyed;
	Game::destroyedCallbacks[static_cast<uint32_t>(InstanceClass::SPAWN_LOCATION)]
			= Geometry::on_destroyed;

	Game::destroyedCallbacks[static_cast<uint32_t>(InstanceClass::SKY)]
			= Sky::on_destroyed;
}

