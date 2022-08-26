#include "rigged_mesh_loader.hpp"

#include <cassert>

#include <animation/rig.hpp>

#include <asset/scene_loader.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_factory.hpp>
#include <core/data_model.hpp>
#include <core/geom.hpp>
#include <core/mesh_geom.hpp>
#include <animation/rig_component.hpp>
#include <core/model.hpp>
#include <animation/bone_attachment.hpp>

#include <rendering/rigged_mesh.hpp>

using namespace Game;

static void populate_bones(ECS::Manager& ecs, Rig& rig, ECS::Entity eParent, Bone& bone);

static bool load_mesh(ECS::Manager& ecs, const std::string& name,
		Memory::SharedPtr<RiggedMesh> mesh, ECS::Entity parentModel, ECS::Entity eRig);

bool Game::RiggedMeshLoader::load_scene(ECS::Manager& ecs, const std::string_view& filePath, Asset::SceneLoadResults* loadResults, bool loadAnimations) {

	Asset::SceneLoadResults stackResult;
	if (!loadResults)
	{
		loadResults = &stackResult;
	}
	if (!Asset::load_scene(filePath, Asset::LOAD_RIGGED_MESHES_BIT
			| (Asset::LOAD_ANIMATIONS_BIT * loadAnimations), loadResults)) {
		printf("load scene\n");
		return false;
	}

	if (loadResults->riggedMeshes.empty()) {
		printf("Empty\n");
		return false;
	}

	auto rig = loadResults->riggedMeshes.front().second->get_rig();

	for (auto& [_, mesh] : loadResults->riggedMeshes) {
		if (mesh->get_rig()->get_rig_id() != rig->get_rig_id()) {
			printf("incorrect rig id\n");
			return false;
		}
	}

	ECS::Entity eModel = ECS::INVALID_ENTITY;
	auto* instModel = InstanceFactory::create(ecs, InstanceClass::MODEL, eModel);

	if (!instModel) {
		printf("failed to create\n");
		return false;
	}

	instModel->m_name = "ImportedRig";

	ECS::Entity eRig = ecs.create_entity();
	ecs.add_component<RigComponent>(eRig, rig, eModel);

	ECS::Entity eRootPart = ECS::INVALID_ENTITY;
	if (!InstanceFactory::create(ecs, InstanceClass::CUBE_GEOM, eRootPart)) {
		return false;
	}

	auto& rootPart = ecs.get_component<Geometry>(eRootPart);
	rootPart.set_shape(eRootPart, GeomType::BLOCK);
	rootPart.set_transform(eRootPart, Math::Transform(1.f));
	rootPart.set_size(eRootPart, Math::Vector3(2.f, 2.f, 1.f));
	rootPart.set_color(eRootPart, Math::Color3uint8(0xFF'7F'7F'7Fu));
	rootPart.set_transparency(eRootPart, 1.f);

	rig->for_each_root([&](auto, auto& bone) {
		populate_bones(ecs, *rig, eRootPart, bone);
	});

	ecs.get_component<Instance>(eRootPart).set_parent(ecs, eModel, eRootPart);

	auto& model = ecs.get_component<Model>(eModel);
	model.set_primary_part(eRootPart);

	ECS::Entity eAnimator = ECS::INVALID_ENTITY;
	auto* instAnimator = InstanceFactory::create(ecs, InstanceClass::ANIMATION_CONTROLLER, eAnimator);

	if (!instAnimator) {
		return false;
	}

	instAnimator->set_parent(ecs, eModel, eAnimator);

	for (auto& [meshName, mesh] : loadResults->riggedMeshes) {
		if (!load_mesh(ecs, meshName, mesh, eModel, eRig)) {
			return false;
		}
	}

	auto eWorkspace = ECS::INVALID_ENTITY;
	g_game->get_singleton(InstanceClass::GAMEWORLD, eWorkspace);
	ecs.get_component<Instance>(eModel).set_parent(ecs, eWorkspace, eModel);

	return true;
}

static void populate_bones(ECS::Manager& ecs, Rig& rig, ECS::Entity eParent, Bone& bone) {
	auto eBone = ECS::INVALID_ENTITY;
	auto* instBA = InstanceFactory::create(ecs, InstanceClass::BONE, eBone);
	assert(instBA && "Failed to create bone attachment");
	auto& ba = ecs.get_component<BoneAttachment>(eBone);

	instBA->m_name = bone.name;
	ba.set_local_transform(ecs, *instBA, Math::Transform(bone.localTransform));

	instBA->set_parent(ecs, eParent, eBone);

	rig.for_each_child(bone, [&](auto, auto& child) {
		populate_bones(ecs, rig, eBone, child);
	});
}

static bool load_mesh(ECS::Manager& ecs, const std::string& name,
		Memory::SharedPtr<RiggedMesh> mesh, ECS::Entity parentModel, ECS::Entity eRig) {
	ECS::Entity eMeshPart = ECS::INVALID_ENTITY;
	auto* instMeshPart = InstanceFactory::create(ecs, InstanceClass::MESH_GEOM, eMeshPart);

	if (!instMeshPart) {
		return false;
	}

	auto& meshPart = ecs.get_component<MeshGeom>(eMeshPart);

	instMeshPart->m_name = name;

	meshPart.set_rigged_mesh(ecs, eMeshPart, name, std::move(mesh), eRig);
	meshPart.set_transform(eMeshPart, Math::Transform(1.f)); // FIXME
	meshPart.set_size(eMeshPart, Math::Vector3(1.f, 1.f, 1.f)); // FIXME
	meshPart.set_color(eMeshPart, Math::Color3uint8{0xFF'FF'FF'FFu});

	instMeshPart->set_parent(ecs, parentModel, eMeshPart);

	return true;
}

