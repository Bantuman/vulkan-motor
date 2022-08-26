#include "attachment.hpp"

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/geom.hpp>
#include <core/mesh_geom.hpp>

using namespace Game;

void Attachment::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Attachment>(entity);
}

void Attachment::on_ancestry_changed(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		Instance*, ECS::Entity) {
	auto& at = ecs.get_component<Attachment>(selfEntity);
	at.recalc_world_transform(ecs, selfInst);
}

void Attachment::set_local_transform(ECS::Manager& ecs, Instance& selfInst, Math::Transform transform) {
	m_localTransform = std::move(transform);
	recalc_world_transform(ecs, selfInst);
}

const Math::Transform& Attachment::get_local_transform() const {
	return m_localTransform;
}

const Math::Transform& Attachment::get_world_transform() const {
	return m_worldTransform;
}

void Attachment::recalc_world_transform(ECS::Manager& ecs, Instance& selfInst) {
	m_worldTransform = m_localTransform;

	for_each_ancestor_cond(ecs, selfInst, [&](auto entity, auto& anc) {
		if (anc.m_classID == InstanceClass::MESH_GEOM) {
			auto& mp = ecs.get_component<MeshGeom>(entity);
			m_worldTransform = mp.get_transform() * m_worldTransform;
		}
		else if (anc.is_a(InstanceClass::BASE_GEOM)) {
			auto& geom = ecs.get_component<Geometry>(entity);
			m_worldTransform = geom.get_transform() * m_worldTransform;
		}
		else if (anc.m_classID == InstanceClass::ATTACHMENT) {
			auto& at = ecs.get_component<Attachment>(entity);
			m_worldTransform = at.get_world_transform() * m_worldTransform;
		}
		else {
			return IterationDecision::CONTINUE;
		}

		return IterationDecision::BREAK;
	});

	update_descendant_world_transforms(ecs, selfInst, *this);
}

void Attachment::update_descendant_world_transforms(ECS::Manager& ecs, Instance& selfInst,
		const Attachment& parent) {
	for_each_child(ecs, selfInst, [&](auto entity, auto& child) {
		if (child.m_classID == InstanceClass::ATTACHMENT) {
			auto& at = ecs.get_component<Attachment>(entity);
			at.m_worldTransform = parent.m_worldTransform * at.m_localTransform;

			update_descendant_world_transforms(ecs, child, at);
		}
	});
}

