#include "game.hpp"

#include <asset/cube_map_cache.hpp>

#include <core/hashed_string.hpp>

#include <ecs/ecs.hpp>

#include <core/components.hpp>
#include <rendering/renderer/game_renderer.hpp>
#include <core/instance.hpp>

using namespace Game;

Instance* Game::get_singleton(ECS::Manager& ecs, InstanceClass classID, ECS::Entity& outEntity) {
	outEntity = ECS::INVALID_ENTITY;
	Instance* result = nullptr;

	ecs.get_view<Instance, ECS::Tag<"Singleton"_hs>>().for_each_cond([&](auto entity, auto& inst,
			auto&) {
		if (inst.m_classID == classID) {
			outEntity = entity;
			result = &inst;
			return IterationDecision::BREAK;
		}

		return IterationDecision::CONTINUE;
	});

	return result;
}

void Game::init_ambience(ECS::Manager& ecs, GameRenderer& renderer) {
	auto enAmbience = ECS::INVALID_ENTITY;
	Game::get_singleton(ecs, InstanceClass::AMBIENCE, enAmbience);

	auto& ambience = ecs.get_component<Game::Ambience>(enAmbience);

	renderer.set_sunlight_dir(ambience.get_sun_direction());
	renderer.set_brightness(ambience.get_brightness());

	auto enSky = ecs.get_component<Instance>(enAmbience).find_first_child_of_class(ecs,
			Game::InstanceClass::SKY);

	if (enSky != ECS::INVALID_ENTITY) {
		auto& sky = ecs.get_component<Game::Sky>(enSky);

		std::string_view skyboxFileNames[] = {
			sky.get_front_id(),
			sky.get_back_id(),
			sky.get_up_id(),
			sky.get_down_id(),
			sky.get_right_id(),
			sky.get_left_id(),
		};

		auto cubeMap = g_cubeMapCache->get_or_load<CubeMapLoader>("Skybox",
				renderer.get_context(), skyboxFileNames, 6, false);
		renderer.set_skybox(std::move(cubeMap));
	}
}

