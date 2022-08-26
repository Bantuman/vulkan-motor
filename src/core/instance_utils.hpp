#pragma once

#include <core/common.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>

#include <string_view>

namespace Game {

template <typename Functor>
inline void for_each_child(ECS::Manager& ecs, Instance& instance, Functor&& func) {
	auto nextEntity = instance.m_firstChild;

	while (nextEntity != ECS::INVALID_ENTITY) {
		auto& child = ecs.get_component<Instance>(nextEntity);

		func(nextEntity, child);

		nextEntity = child.m_nextChild;
	}
}

template <typename Functor>
inline void for_each_child_cond(ECS::Manager& ecs, Instance& instance, Functor&& func) {
	auto nextEntity = instance.m_firstChild;

	while (nextEntity != ECS::INVALID_ENTITY) {
		auto& child = ecs.get_component<Instance>(nextEntity);

		if (func(nextEntity, child) == IterationDecision::BREAK) {
			return;
		}

		nextEntity = child.m_nextChild;
	}
}

template <typename Functor>
inline void for_each_descendant(ECS::Manager& ecs, Instance& instance, Functor&& func) {
	for_each_child(ecs, instance, [&](auto entity, auto& child) {
		func(entity, child);
		for_each_descendant(ecs, child, func);
	});
}

template <typename Functor>
inline void for_each_ancestor(ECS::Manager& ecs, Instance& instance, Functor&& func) {
	auto parentEntity = instance.m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		auto& parent = ecs.get_component<Instance>(parentEntity);

		func(parentEntity, parent);

		parentEntity = parent.m_parent;
	}
}

template <typename Functor>
inline void for_each_ancestor_cond(ECS::Manager& ecs, Instance& instance, Functor&& func) {
	auto parentEntity = instance.m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		auto& parent = ecs.get_component<Instance>(parentEntity);

		if (func(parentEntity, parent) == IterationDecision::BREAK) {
			return;
		}

		parentEntity = parent.m_parent;
	}
}

InstanceClass get_instance_class_id_by_name(const std::string_view& name);
std::string_view get_instance_class_name(InstanceClass);
bool instance_class_is_a(InstanceClass childToTest, InstanceClass baseClass);
bool instance_class_is_creatable(InstanceClass);

}

