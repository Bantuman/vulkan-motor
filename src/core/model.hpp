#pragma once

#include <ecs/ecs_fwd.hpp>

#include <math/transform.hpp>

namespace Game {

struct Instance;

class Model {
	public:
		static void create(ECS::Manager&, ECS::Entity);

		void set_primary_part(ECS::Entity);

		void set_primary_geom_transform(ECS::Manager&, Instance& selfInstance, const Math::Transform&);

		ECS::Entity get_primary_part() const;
	private:
		ECS::Entity m_primaryPart = ECS::INVALID_ENTITY;
};

}

