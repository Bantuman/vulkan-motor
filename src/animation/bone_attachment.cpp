#include "bone_attachment.hpp"

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/geom.hpp>
#include <core/mesh_geom.hpp>

using namespace Game;

void BoneAttachment::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<BoneAttachment>(entity);
}

void BoneAttachment::on_ancestry_changed(ECS::Manager& ecs, Instance& selfInst,
		ECS::Entity selfEntity, Instance*, ECS::Entity) {
	auto& at = ecs.get_component<BoneAttachment>(selfEntity);
	at.recalc_world_transform(ecs, selfInst);
}

void BoneAttachment::set_local_transform(ECS::Manager& ecs, Instance& selfInst, Math::Transform transform) {
	m_localTransform = std::move(transform);
	m_transformedTransform = m_localTransform * m_transform;
	recalc_world_transform(ecs, selfInst);
}

void BoneAttachment::set_transform(ECS::Manager& ecs, Instance& selfInst, Math::Transform transform) {
	m_transform = std::move(transform);
	m_transformedTransform = m_localTransform * m_transform;
	recalc_world_transform(ecs, selfInst);
}

const Math::Transform& BoneAttachment::get_local_transform() const {
	return m_localTransform;
}

const Math::Transform& BoneAttachment::get_world_transform() const {
	return m_worldTransform;
}

const Math::Transform& BoneAttachment::get_transform() const {
	return m_transform;
}

const Math::Transform& BoneAttachment::get_transformed_transform() const {
	return m_transformedTransform;
}

const Math::Transform& BoneAttachment::get_transformed_world_transform() const {
	return m_transformedWorldTransform;
}

void BoneAttachment::recalc_world_transform(ECS::Manager& ecs, Instance& selfInst) {
	m_worldTransform = m_localTransform;
	m_transformedWorldTransform = m_transformedTransform;

	for_each_ancestor_cond(ecs, selfInst, [&](auto entity, auto& anc) {
		if (anc.m_classID == InstanceClass::MESH_GEOM) {
			auto& mp = ecs.get_component<MeshGeom>(entity);
			m_worldTransform = mp.get_transform() * m_worldTransform;
			m_transformedWorldTransform = mp.get_transform() * m_transformedTransform;
		}
		else if (anc.is_a(InstanceClass::BASE_GEOM)) {
			auto& geom = ecs.get_component<Geometry>(entity);
			m_worldTransform = geom.get_transform() * m_worldTransform;
			m_transformedWorldTransform = geom.get_transform() * m_transformedTransform;
		}
		else if (anc.m_classID == InstanceClass::BONE) {
			auto& at = ecs.get_component<BoneAttachment>(entity);
			m_worldTransform = at.get_world_transform() * m_worldTransform;
			m_transformedWorldTransform = at.get_transformed_world_transform() * m_transformedTransform;
		}
		else {
			return IterationDecision::CONTINUE;
		}

		return IterationDecision::BREAK;
	});

	update_descendant_world_transforms(ecs, selfInst, *this);
}

void BoneAttachment::update_descendant_world_transforms(ECS::Manager& ecs, Instance& selfInst,
		const BoneAttachment& parent) {
	for_each_child(ecs, selfInst, [&](auto entity, auto& child) {
		if (child.m_classID == InstanceClass::BONE) {
			auto& at = ecs.get_component<BoneAttachment>(entity);
			at.m_worldTransform = parent.m_worldTransform * at.m_localTransform;
			at.m_transformedWorldTransform = parent.m_transformedWorldTransform
					* at.m_transformedTransform;

			update_descendant_world_transforms(ecs, child, at);
		}
	});
}

