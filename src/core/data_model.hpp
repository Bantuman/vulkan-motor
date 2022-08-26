#pragma once

#include <core/common.hpp>
#include <core/memory.hpp>

#include <ecs/ecs_fwd.hpp>

#include <core/instance_class.hpp>

namespace Game {

struct Instance;
class Gameworld;

class DataModel final {
	public:
		explicit DataModel(ECS::Manager&);

		NULL_COPY_AND_ASSIGN(DataModel);

		void set_gameworld(ECS::Entity);

		Instance* get_singleton(InstanceClass, ECS::Entity& outEntity);

		ECS::Entity get_self_entity() const;
		ECS::Entity get_gameworld_entity() const;

		ECS::Manager& get_ecs();
		Gameworld& get_gameworld();
	private:
		ECS::Manager& m_ecs;

		ECS::Entity m_selfEntity;
		ECS::Entity m_gameworld;

		void init_services();
};

inline std::shared_ptr<DataModel> g_game;

}

