#include "sky.hpp"

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/data_model.hpp>
#include <core/ambience.hpp>

using namespace Game;

static void set_next_skybox(ECS::Manager& ecs, Ambience& lighting, Instance& instLighting);

void Sky::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Sky>(entity);
}

void Sky::on_destroyed(ECS::Manager& ecs, Instance&, ECS::Entity entity) {
	ECS::Entity eLighting;
	auto* instLighting = g_game->get_singleton(InstanceClass::AMBIENCE, eLighting);
	auto& lighting = ecs.get_component<Ambience>(eLighting);

	if (lighting.get_active_skybox() == entity) {
		set_next_skybox(ecs, lighting, *instLighting);
	}
}

void Sky::on_ancestry_changed(ECS::Manager& ecs, Instance&, ECS::Entity entity,
		Instance* parentInst, ECS::Entity) {
	ECS::Entity eLighting;
	auto* instLighting = g_game->get_singleton(InstanceClass::AMBIENCE, eLighting);
	auto& lighting = ecs.get_component<Ambience>(eLighting);

	if (parentInst && parentInst->m_classID == InstanceClass::AMBIENCE) {
		if (lighting.get_active_skybox() == ECS::INVALID_ENTITY) {
			lighting.set_active_skybox(entity, &ecs.get_component<Sky>(entity));
		}
	}
	else if (lighting.get_active_skybox() == entity) {
		set_next_skybox(ecs, lighting, *instLighting);
	}
}

void Sky::set_back_id(std::string id) {
	m_skyboxBackID = std::move(id);
}


void Sky::set_front_id(std::string id) {
	m_skyboxFrontID = std::move(id);
}

void Sky::set_left_id(std::string id) {
	m_skyboxLeftID = std::move(id);
}

void Sky::set_right_id(std::string id) {
	m_skyboxRightID = std::move(id);
}

void Sky::set_up_id(std::string id) {
	m_skyboxUpID = std::move(id);
}

void Sky::set_down_id(std::string id) {
	m_skyboxDownID = std::move(id);
}

const std::string& Sky::get_back_id() const {
	return m_skyboxBackID;
}

const std::string& Sky::get_front_id() const {
	return m_skyboxFrontID;
}

const std::string& Sky::get_left_id() const {
	return m_skyboxLeftID;
}

const std::string& Sky::get_right_id() const {
	return m_skyboxRightID;
}

const std::string& Sky::get_up_id() const {
	return m_skyboxUpID;
}

const std::string& Sky::get_down_id() const {
	return m_skyboxDownID;
}

static void set_next_skybox(ECS::Manager& ecs, Ambience& lighting, Instance& instLighting) {
	auto eNextSky = ECS::INVALID_ENTITY;
	const Sky* nextSky = nullptr;

	for_each_child_cond(ecs, instLighting, [&](auto eChild, auto& child) {
		if (child.m_classID == InstanceClass::SKY) {
			eNextSky = eChild;
			nextSky = &ecs.get_component<Sky>(eChild);

			return IterationDecision::BREAK;
		}

		return IterationDecision::CONTINUE;
	});

	lighting.set_active_skybox(eNextSky, nextSky);
}

