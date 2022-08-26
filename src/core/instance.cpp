#include "instance.hpp"

#include <ecs/ecs.hpp>

#include <core/instance_utils.hpp>
#include <core/ancestry_changed_callbacks.hpp>
#include <core/destroyed_callbacks.hpp>

using namespace Game;

static void dispatch_ancestry_change(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		Instance* oldParentInst, ECS::Entity oldParentEntity, Instance* newParentInst,
		ECS::Entity newParentEntity);

void Game::Instance::set_parent(ECS::Manager& ecs, ECS::Entity newParentEntity,
		ECS::Entity selfEntity) {
	if (newParentEntity == m_parent || newParentEntity == selfEntity) {
		return;
	}

	auto oldParentEntity = m_parent;
	m_parent = newParentEntity;

	Instance* oldParent = nullptr;
	Instance* newParent = nullptr;

	if (m_lastChild != ECS::INVALID_ENTITY) {
		auto& lastChildInstance = ecs.get_component<Instance>(m_lastChild);
		lastChildInstance.m_nextChild = m_nextChild;
	}

	if (oldParentEntity != ECS::INVALID_ENTITY) {
		auto& oldParentInstance = ecs.get_component<Instance>(oldParentEntity);
		oldParent = &oldParentInstance;

		if (oldParentInstance.m_firstChild == selfEntity) {
			oldParentInstance.m_firstChild = m_nextChild;
		}
		if (oldParentInstance.m_lastChild == selfEntity) {
			oldParentInstance.m_lastChild = m_nextChild;
		}
	}

	if (newParentEntity != ECS::INVALID_ENTITY) {
		auto& parentInstance = ecs.get_component<Instance>(newParentEntity);
		newParent = &parentInstance;

		if (parentInstance.m_lastChild != ECS::INVALID_ENTITY) {
			auto& lastChild = ecs.get_component<Instance>(parentInstance.m_lastChild);
			lastChild.m_nextChild = selfEntity;
		}
		else {
			parentInstance.m_firstChild = selfEntity;
		}

		m_nextChild = ECS::INVALID_ENTITY;
		parentInstance.m_lastChild = selfEntity;
	}

	dispatch_ancestry_change(ecs, *this, selfEntity, oldParent, oldParentEntity, newParent,
			newParentEntity);
}

void Game::Instance::destroy(ECS::Manager& ecs, ECS::Entity selfEntity) {
	if (m_destroyed) {
		return;
	}

	m_destroyed = true;

	if (auto callback = Game::destroyedCallbacks[static_cast<uint32_t>(m_classID)]; callback) {
		callback(ecs, *this, selfEntity);
	}

	ecs.destroy_entity(selfEntity);
}

ECS::Entity Game::Instance::find_first_child(ECS::Manager& ecs,
		const std::string_view& name) const {
	auto nextEntity = m_firstChild;

	while (nextEntity != ECS::INVALID_ENTITY) {
		auto& child = ecs.get_component<Instance>(nextEntity);

		if (name.compare(child.m_name) == 0) {
			return nextEntity;
		}

		nextEntity = child.m_nextChild;
	}

	return ECS::INVALID_ENTITY;
}

ECS::Entity Game::Instance::find_first_child_of_class(ECS::Manager& ecs,
		InstanceClass type) const {
	auto nextEntity = m_firstChild;

	while (nextEntity != ECS::INVALID_ENTITY) {
		auto& child = ecs.get_component<Instance>(nextEntity);

		if (child.m_classID == type) {
			return nextEntity;
		}

		nextEntity = child.m_nextChild;
	}

	return ECS::INVALID_ENTITY;
}

std::string_view Game::Instance::get_class_name() const {
	return Game::get_instance_class_name(m_classID);
}

std::string Game::Instance::get_full_name(ECS::Manager& ecs) {
	std::string result = m_name;

	Game::for_each_ancestor(ecs, *this, [&](auto, auto& inst) {
		result = inst.m_name + "." + result;
	});

	return result;
}

bool Game::Instance::is_a(Game::InstanceClass id) const {
	return Game::instance_class_is_a(m_classID, id);
}

bool Game::Instance::is_descendant_of(ECS::Manager& ecs, ECS::Entity ancestor) const {
	auto parentEntity = m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		if (parentEntity == ancestor) {
			return true;
		}

		auto& parent = ecs.get_component<Instance>(parentEntity);
		parentEntity = parent.m_parent;
	}

	return false;
}

bool Game::Instance::is_ancestor_of(ECS::Manager& ecs, ECS::Entity selfEntity,
		const Instance& descendant) const {
	return descendant.is_descendant_of(ecs, selfEntity);
}

ECS::Entity Game::Instance::find_first_ancestor(ECS::Manager& ecs,
		const std::string_view& name) const {
	auto parentEntity = m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		auto& parent = ecs.get_component<Game::Instance>(parentEntity);

		if (name.compare(parent.m_name) == 0) {
			return parentEntity;
		}

		parentEntity = parent.m_parent;
	}

	return ECS::INVALID_ENTITY;
}

ECS::Entity Game::Instance::find_first_ancestor_of_class(ECS::Manager& ecs,
		Game::InstanceClass id) const {
	auto parentEntity = m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		auto& parent = ecs.get_component<Game::Instance>(parentEntity);

		if (parent.m_classID == id) {
			return parentEntity;
		}

		parentEntity = parent.m_parent;
	}

	return ECS::INVALID_ENTITY;
}

ECS::Entity Game::Instance::find_first_ancestor_which_is_a(ECS::Manager& ecs,
		Game::InstanceClass id) const {
	auto parentEntity = m_parent;

	while (parentEntity != ECS::INVALID_ENTITY) {
		auto& parent = ecs.get_component<Game::Instance>(parentEntity);

		if (parent.is_a(id)) {
			return parentEntity;
		}

		parentEntity = parent.m_parent;
	}

	return ECS::INVALID_ENTITY;
}

//#include <core/logging.hpp>

static void dispatch_ancestry_change(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		Instance* oldParentInst, ECS::Entity oldParentEntity, Instance* newParentInst,
		ECS::Entity newParentEntity) {
	(void)oldParentInst;
	(void)oldParentEntity;

	/*if (oldParentInst) {
		LOG_TEMP("%s.ChildRemoved(%s)", oldParentInst->get_full_name(ecs).c_str(),
				selfInst.get_full_name(ecs).c_str());

		if (!selfInst.is_descendant_of(ecs, oldParentEntity)) {
			for_each_descendant(ecs, selfInst, [&](auto, auto& desc) {
				LOG_TEMP("%s.DescendantRemoving(%s)", oldParentInst->get_full_name(ecs).c_str(),
						desc.get_full_name(ecs).c_str());
			});

			for_each_ancestor(ecs, *oldParentInst, [&](auto, auto& ancestor) {
				LOG_TEMP("%s.DescendantRemoving(%s)", ancestor.get_full_name(ecs).c_str(),
						selfInst.get_full_name(ecs).c_str());

				for_each_descendant(ecs, selfInst, [&](auto, auto& desc) {
					LOG_TEMP("%s.DescendantRemoving(%s)",
							ancestor.get_full_name(ecs).c_str(),
							desc.get_full_name(ecs).c_str());
				});
			});
		}
	}

	if (newParentInst) {
		LOG_TEMP("%s.DescendantAdded(%s)", newParentInst->get_full_name(ecs).c_str(),
				selfInst.get_full_name(ecs).c_str());

		if (!(oldParentInst && oldParentInst->is_descendant_of(ecs, newParentEntity))) {
			for_each_descendant(ecs, selfInst, [&](auto, auto& desc) {
				LOG_TEMP("%s.DescendantAdded(%s)", newParentInst->get_full_name(ecs).c_str(),
						desc.get_full_name(ecs).c_str());
			});

			for_each_ancestor_cond(ecs, *newParentInst, [&](auto ancEntity, auto& ancestor) {
				if (oldParentEntity == ancEntity
						|| (oldParentInst && oldParentInst->is_descendant_of(ecs, ancEntity))) {
					return IterationDecision::BREAK;
				}

				LOG_TEMP("%s.DescendantAdded(%s)", ancestor.get_full_name(ecs).c_str(),
						selfInst.get_full_name(ecs).c_str());

				for_each_descendant(ecs, selfInst, [&](auto, auto& desc) {
					LOG_TEMP("%s.DescendantAdded(%s)", ancestor.get_full_name(ecs).c_str(),
							desc.get_full_name(ecs).c_str());
				});

				return IterationDecision::CONTINUE;
			});
		}

		LOG_TEMP("%s.ChildAdded(%s)", newParentInst->get_full_name(ecs).c_str(),
				selfInst.get_full_name(ecs).c_str());
	}*/

	if (auto callback = ancestryChangedCallbacks[static_cast<uint32_t>(selfInst.m_classID)];
			callback) {
		callback(ecs, selfInst, selfEntity, newParentInst, newParentEntity);
	}

	for_each_descendant(ecs, selfInst, [&](auto childEntity, auto& child) {
		if (auto callback = ancestryChangedCallbacks[static_cast<uint32_t>(child.m_classID)];
				callback) {
			callback(ecs, child, childEntity, newParentInst, newParentEntity);
		}
	});

	/*LOG_TEMP("%s.AncestryChanged(child: %s, parent: %s)", selfInst.get_full_name(ecs).c_str(),
			selfInst.get_full_name(ecs).c_str(),
			newParentInst ? newParentInst->get_full_name(ecs).c_str() : "nil");

	for_each_descendant(ecs, selfInst, [&](auto, auto& child) {
		LOG_TEMP("%s.AncestryChanged(child: %s, parent: %s)", child.get_full_name(ecs).c_str(),
				selfInst.get_full_name(ecs).c_str(),
				newParentInst ? newParentInst->get_full_name(ecs).c_str() : "nil");
	});*/
}

