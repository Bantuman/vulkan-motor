#include "model.hpp"
#include "glm/gtx/string_cast.hpp"
#include <unordered_map>

#include <core/pair.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/geom.hpp>
#include <core/mesh_geom.hpp>

using namespace Game;

void Model::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Model>(entity);
}

void Model::set_primary_part(ECS::Entity entity) {
	m_primaryPart = entity;
}

void Model::set_primary_geom_transform(ECS::Manager& ecs, Instance& selfInstance,
		const Math::Transform& transform) {
	if (!ecs.is_valid_entity(m_primaryPart)) {
		return;
	}

	// FIXME: frame memory
	std::unordered_map<Geometry*, Pair<ECS::Entity, Math::Transform>> parts;
	std::unordered_map<MeshGeom*, Pair<ECS::Entity, Math::Transform>> meshParts;

	auto& instPrimaryPart = ecs.get_component<Instance>(m_primaryPart);
	Math::Transform invPrimaryPartCF;

	if (instPrimaryPart.m_classID == InstanceClass::MESH_GEOM) {
		invPrimaryPartCF = ecs.get_component<MeshGeom>(m_primaryPart).get_transform().fast_inverse();
	}
	else {
		invPrimaryPartCF = ecs.get_component<Geometry>(m_primaryPart).get_transform().fast_inverse();
	}
	
	//printf("invPrimaryPartCF -> %s\n", glm::to_string(invPrimaryPartCF.to_matrix4x4()).data());
	for_each_descendant(ecs, selfInstance, [&](auto entity, auto& desc) {
		if (desc.m_classID == InstanceClass::MESH_GEOM) {
			auto& mp = ecs.get_component<MeshGeom>(entity);
			meshParts.emplace(std::make_pair(&mp,
					Pair{entity, invPrimaryPartCF * mp.get_transform()}));
		}
		else if (desc.is_a(InstanceClass::BASE_GEOM)) {
			auto& pt = ecs.get_component<Geometry>(entity);
			parts.emplace(std::make_pair(&pt,
					Pair{entity, invPrimaryPartCF * pt.get_transform()}));
		}
	});

	for (auto& [pPart, cfPair] : parts) {
		pPart->set_transform(cfPair.first, transform * cfPair.second);
	}

	for (auto& [pPart, cfPair] : meshParts) {
		pPart->set_transform(cfPair.first, transform * cfPair.second);
	}
	
	if (instPrimaryPart.m_classID == InstanceClass::MESH_GEOM) {
		ecs.get_component<MeshGeom>(m_primaryPart).set_transform(m_primaryPart, transform);
	}
	else {
		ecs.get_component<Geometry>(m_primaryPart).set_transform(m_primaryPart, transform);
	}
}

ECS::Entity Model::get_primary_part() const {
	return m_primaryPart;
}

