#pragma once

#include <core/instance_class.hpp>

#include <ecs/ecs_fwd.hpp>

namespace Game {

struct InstanceHandle {
	constexpr InstanceHandle()
			: classID(InstanceClass::INSTANCE)
			, entity(ECS::INVALID_HANDLE) {}
	constexpr InstanceHandle(InstanceClass classIDIn, ECS::Entity entityIn)
			: classID(classIDIn)
			, entity(entityIn) {}

	InstanceClass classID;
	ECS::Entity entity;
};

Instance& get_instance_by_handle(ECS::Manager&, InstanceHandle);
Instance* try_get_instance_by_handle(ECS::Manager&, InstanceHandle);

}

