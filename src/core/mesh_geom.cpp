#include "mesh_geom.hpp"

#include <asset/texture_cache.hpp>

#include <animation/rig.hpp>
#include <rendering/renderer/rigged_mesh_renderer.hpp>

#include <ecs/ecs.hpp>

#include <core/mesh_geom_instance.hpp>
#include <animation/rig_component.hpp>

#include <math/color3.hpp>

#include <rendering/mesh.hpp>
#include <rendering/rigged_mesh.hpp>

using namespace Game;

void MeshGeom::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<MeshGeom>(entity);
}

void MeshGeom::set_transform(ECS::Entity selfEntity, Math::Transform transform) {
	m_transform = std::move(transform);

	if (!is_visible()) {
		return;
	}

	auto mesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);
	auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, m_transparency == 0.f,
			selfEntity);
	inst.m_transform = m_transform;
}

void MeshGeom::set_size(ECS::Entity selfEntity, Math::Vector3 size) {
	m_size = std::move(size);
	m_scale = m_size / m_originalSize;

	if (!is_visible()) {
		return;
	}

	auto mesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);
	auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, m_transparency == 0.f,
			selfEntity);
	inst.m_transform = m_transform.scale_by(m_scale);
}

void MeshGeom::set_color(ECS::Entity selfEntity, Math::Color3uint8 color) {
	if (m_color == color) {
		return;
	}

	m_color = std::move(color);

	if (!is_visible()) {
		return;
	}

	auto mesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);
	auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, m_transparency == 0.f,
			selfEntity);
	inst.m_color = Math::Color3(m_color).to_vector4(1.f - m_transparency);
}

void MeshGeom::set_transparency(ECS::Entity selfEntity, float transparency) {
	if (m_transparency == transparency) {
		return;
	}

	auto mesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);

	if (transparency == 1.f) {
		g_riggedMeshRenderer->remove_instance(mesh, m_transparency == 0.f, selfEntity);
	}
	else if (transparency == 0.f || m_transparency == 0.f) {
		auto& oldInst = g_riggedMeshRenderer->get_or_add_instance(mesh, m_transparency == 0.f,
				selfEntity);
		auto& newInst = g_riggedMeshRenderer->get_or_add_instance(mesh, transparency == 0.f,
				selfEntity);

		memcpy(&newInst, &oldInst, sizeof(MeshGeomInstance));

		g_riggedMeshRenderer->remove_instance(mesh, m_transparency == 0.f, selfEntity);
	}
	else {
		auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, m_transparency == 0.f,
				selfEntity);
		inst.m_color = Math::Color3(m_color).to_vector4(1.f - transparency);
	}

	m_transparency = transparency;
}

void MeshGeom::set_rigged_mesh(ECS::Manager& ecs, ECS::Entity selfEntity, std::string meshID,
		Memory::SharedPtr<RiggedMesh> mesh, ECS::Entity eRig) {
	if (is_visible()) {
		auto oldMesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);
		g_riggedMeshRenderer->remove_instance(oldMesh, true, selfEntity);
	}

	m_meshID = std::move(meshID);
	m_mesh = mesh;
	m_rig = eRig;
	m_originalSize = Math::Vector3(1, 1, 1);

	if (!is_visible()) {
		return;
	}

	auto& rigPool = ecs.get_pool<RigComponent>();

	auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, true, selfEntity);
	RenderKey key{ mesh->get_rig(), mesh };
	auto& bucket = g_riggedMeshRenderer->get_instances().get_bucket(key);
	size_t index = bucket.get_sparse_index(selfEntity);

	inst.m_transform = m_transform.scale_by(m_scale);
	inst.m_reflectance = m_reflectance;
	inst.m_indices.diffuseTexture = g_riggedMeshRenderer->get_default_diffuse_index();
	inst.m_indices.normalTexture = g_riggedMeshRenderer->get_default_normal_index();
	inst.m_indices.rig = static_cast<uint32_t>(index * mesh->get_rig()->get_num_bones());
	//inst.m_indices.rig = static_cast<uint32_t>(rigPool.get_sparse_index(eRig) * mesh->get_rig()->get_num_bones());
}

void MeshGeom::set_texture(ECS::Entity selfEntity, std::string textureID) {
	if (textureID.compare(m_textureID) == 0) {
		return;
	}

	m_textureID = std::move(textureID);

	if (!is_visible()) {
		return;
	}

	auto texture = g_textureCache->get(m_textureID);

	auto mesh = Memory::static_pointer_cast<RiggedMesh>(m_mesh);
	auto& inst = g_riggedMeshRenderer->get_or_add_instance(mesh, true, selfEntity);

	if (texture) {
		inst.m_indices.diffuseTexture = g_riggedMeshRenderer->get_image_index(
				*texture->get_image_view());
	}
	else {
		inst.m_indices.diffuseTexture = g_riggedMeshRenderer->get_default_diffuse_index();
	}
}

const Math::Transform& MeshGeom::get_transform() const {
	return m_transform;
}

bool MeshGeom::is_visible() const {
	return m_transparency != 1.f && m_mesh;
}

