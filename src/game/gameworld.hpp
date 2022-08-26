#pragma once

#include <ecs/ecs_fwd.hpp>

namespace Game {

class Gameworld final {
	public:
		static void create(ECS::Manager&, ECS::Entity);

		void update(float deltaTime);
		void init();
		void deinit();

		ECS::Entity get_current_camera() const;
		ECS::Entity get_local_player() const;
	private:
		void update_current_camera();
		void set_current_camera(ECS::Entity);
		void set_local_player(ECS::Entity);

		ECS::Entity m_currentCamera = ECS::INVALID_ENTITY;
		ECS::Entity m_localPlayer = ECS::INVALID_ENTITY;
};

}

