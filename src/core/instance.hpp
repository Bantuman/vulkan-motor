#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/instance_class.hpp>

#include <string>

namespace Game {

struct Instance {
	ECS::Entity m_parent = ECS::INVALID_ENTITY;
	ECS::Entity m_firstChild = ECS::INVALID_ENTITY;
	ECS::Entity m_lastChild = ECS::INVALID_ENTITY;
	ECS::Entity m_nextChild = ECS::INVALID_ENTITY;
	ECS::Entity m_prevChild = ECS::INVALID_ENTITY;
	std::string m_name;
	InstanceClass m_classID;
	// FIXME: the only property missing is bool m_archivable;
	bool m_destroyed;

	void set_parent(ECS::Manager&, ECS::Entity parent, ECS::Entity selfEntity);
	void destroy(ECS::Manager&, ECS::Entity selfEntity);

	ECS::Entity find_first_child(ECS::Manager&, const std::string_view& name) const;
	ECS::Entity find_first_child_of_class(ECS::Manager&, InstanceClass) const;

	ECS::Entity find_first_ancestor(ECS::Manager&, const std::string_view& name) const;
	ECS::Entity find_first_ancestor_of_class(ECS::Manager&, InstanceClass) const;
	ECS::Entity find_first_ancestor_which_is_a(ECS::Manager&, InstanceClass) const;

	std::string_view get_class_name() const;
	std::string get_full_name(ECS::Manager&);

	bool is_a(InstanceClass) const;

	bool is_descendant_of(ECS::Manager&, ECS::Entity ancestor) const;
	bool is_ancestor_of(ECS::Manager&, ECS::Entity selfEntity, const Instance& descendant) const;
};

}

