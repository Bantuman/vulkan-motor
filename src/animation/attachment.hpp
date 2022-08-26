#pragma once

#include <ecs/ecs_fwd.hpp>

#include <math/transform.hpp>

namespace Game {

struct Instance;

class Attachment {
	public:
		static void create(ECS::Manager&, ECS::Entity);
		static void on_ancestry_changed(ECS::Manager&, Instance&, ECS::Entity, Instance*,
				ECS::Entity);

		void set_local_transform(ECS::Manager&, Instance& selfInstance, Math::Transform);

		const Math::Transform& get_local_transform() const;
		const Math::Transform& get_world_transform() const;
	private:
		Math::Transform m_localTransform = Math::Transform(1.f);
		Math::Transform m_worldTransform;

		void recalc_world_transform(ECS::Manager&, Instance&);

		static void update_descendant_world_transforms(ECS::Manager&, Instance&, const Attachment&);
};

}

