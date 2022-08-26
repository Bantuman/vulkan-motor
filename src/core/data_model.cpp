#include "data_model.hpp"

#include <cassert>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/instance_factory.hpp>
#include <game/gameworld.hpp>

using namespace Game;

DataModel::DataModel(ECS::Manager& ecs)
		: m_ecs(ecs)
		, m_selfEntity(ecs.create_entity())
		, m_gameworld(ECS::INVALID_ENTITY) {
	auto& inst = ecs.add_component<Instance>(m_selfEntity);
	inst.m_name = "Game";
	inst.m_classID = InstanceClass::GAME;

	init_services();
}

void DataModel::set_gameworld(ECS::Entity eWorkspace) {
	m_gameworld = eWorkspace;
}

Instance* DataModel::get_singleton(InstanceClass classID, ECS::Entity& outEntity) {
	auto& selfInst = m_ecs.get_component<Instance>(m_selfEntity);
	outEntity = ECS::INVALID_ENTITY;
	Instance* result = nullptr;

	for_each_child_cond(m_ecs, selfInst, [&](auto eChild, auto& child) {
		if (child.m_classID == classID) {
			outEntity = eChild;
			result = &child;
			return IterationDecision::BREAK;
		}

		return IterationDecision::CONTINUE;
	});

	return result;
}

ECS::Entity DataModel::get_self_entity() const {
	return m_selfEntity;
}

ECS::Entity DataModel::get_gameworld_entity() const {
	return m_gameworld;
}

ECS::Manager& DataModel::get_ecs() {
	return m_ecs;
}

Gameworld& DataModel::get_gameworld() {
	return m_ecs.get_component<Gameworld>(m_gameworld);
}

void DataModel::init_services() {
	auto* instWorkspace = InstanceFactory::create(m_ecs, InstanceClass::GAMEWORLD, m_gameworld);
	instWorkspace->set_parent(m_ecs, m_selfEntity, m_gameworld);

	ECS::Entity eAmbience;
	auto* instAmbience = InstanceFactory::create(m_ecs, InstanceClass::AMBIENCE, eAmbience);
	instAmbience->set_parent(m_ecs, m_selfEntity, eAmbience);

	ECS::Entity eEngineGui;
	auto* instEngineGui = InstanceFactory::create(m_ecs, InstanceClass::ENGINE_GUI, eEngineGui);
	instEngineGui->set_parent(m_ecs, m_selfEntity, eEngineGui);

	ECS::Entity eGameGui;
	auto* instGameGui = InstanceFactory::create(m_ecs, InstanceClass::GAME_GUI, eGameGui);
	instGameGui->set_parent(m_ecs, m_selfEntity, eGameGui);
}

