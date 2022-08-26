#include "gameworld.hpp"

#include <ecs/ecs_fwd.hpp>
#include <ecs/ecs.hpp>

#include "core/instance.hpp"
#include "core/instance_factory.hpp"
#include "rendering/renderer/game_renderer.hpp"
#include "core/data_model.hpp"
#include "core/camera.hpp"
#include "game/player.hpp"
#include "asset/rigged_mesh_loader.hpp"
#include "core/model.hpp"
#include "core/mesh_geom.hpp"
#include "core/instance_utils.hpp"

#include <asset/scene_loader.hpp>
#include <core/logging.hpp>
#include <core/hashed_string.hpp>
#include <asset/texture_cache.hpp>

using namespace Game;

ECS::Entity LoadAndInitializeMesh(const char* meshPath, const char* modelName, const std::vector<Pair<const char*, const char*>> textureMap)
{
	if (!Game::RiggedMeshLoader::load_scene(*g_ecs, meshPath, nullptr, false)) {
		LOG_TEMP2("Failed to load rigged mesh");
	}
	auto eWorkspace = ECS::INVALID_ENTITY;
	auto* instWorkspace = Game::g_game->get_singleton(Game::InstanceClass::GAMEWORLD, eWorkspace);

	auto eImportedRig = instWorkspace->find_first_child(*g_ecs, "ImportedRig");
	auto& instImportedRig = g_ecs->get_component<Game::Instance>(eImportedRig);
	
	for (auto& [name, path] : textureMap)
	{
		g_textureCache->get_or_load<TextureLoader>(name, *g_renderContext, path, false, true);
		for_each_descendant(*g_ecs, instImportedRig, [&](auto entity, auto& desc) {
			if (desc.m_classID == Game::InstanceClass::MESH_GEOM) {
				auto& mp = g_ecs->get_component<Game::MeshGeom>(entity);
				//mp.set_size(entity, Math::Vector3(10, 10, 10));

				if (desc.m_name.compare(name) == 0)
				{
					mp.set_texture(entity, name);
				}
			}
		});
	}
	
	instImportedRig.m_name = modelName;
	return eImportedRig;
}


void Gameworld::init()
{
	ECS::Entity eCamera;
	InstanceFactory::create(*g_ecs, InstanceClass::CAMERA, eCamera);
	auto& camera = g_ecs->get_component<Camera>(eCamera);
	camera.set_field_of_view(70.f);
	camera.set_transform(Math::Transform({ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }));
	camera.set_focus(Math::Transform({ 0, 0, 0.1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }));

	set_current_camera(eCamera);

	ECS::Entity player = g_ecs->create_entity();
	g_ecs->add_component<PlayerController>(player);
	set_local_player(player);

	{
		auto eSU47E = LoadAndInitializeMesh("res://TSF/SU47E/Craft/Body.glb", "SU47E", { {"su47E", "res://TSF/SU47E/Craft/Chara_tsf_30000_su47e_body_color.png"} });
		{
			auto& model = g_ecs->get_component<Model>(eSU47E);
			auto& instance = g_ecs->get_component<Instance>(eSU47E);
			model.set_primary_geom_transform(*g_ecs, instance, Math::Transform(0, 5, 0) * Math::Transform::from_axis_angle({ 1, 0, 0 }, Math::radians<float>(90)));
		}
		eSU47E = LoadAndInitializeMesh("res://TSF/SU47E/Craft/Body.glb", "SU47E", { {"su47E", "res://TSF/SU47E/Craft/Chara_tsf_30000_su47e_body_color.png"} });
		{
			auto& model = g_ecs->get_component<Model>(eSU47E);
			auto& instance = g_ecs->get_component<Instance>(eSU47E);
			model.set_primary_geom_transform(*g_ecs, instance, Math::Transform(25, 5, 0) * Math::Transform::from_axis_angle({ 1, 0, 0 }, Math::radians<float>(90)));
		}
		auto eSU37 = LoadAndInitializeMesh("res://TSF/SU37/Craft/Body.glb", "SU37", { {"su37", "res://TSF/SU37/Craft/Chara_tsf_033001_su37m2_body_color.png"} });
		{
			auto& model = g_ecs->get_component<Model>(eSU37);
			auto& instance = g_ecs->get_component<Instance>(eSU37);
			model.set_primary_geom_transform(*g_ecs, instance, Math::Transform(50, 5, 0) * Math::Transform::from_axis_angle({ 1, 0, 0 }, Math::radians<float>(90)));
		}
		//g_ecs->add_component<ECS::Tag<"CharacterMesh"_hs>>(su47e);
	}
}


void Gameworld::update(float deltaTime)
{
	deltaTime;
	//update_player_controls(deltaTime);
	//update_player(deltaTime);
	update_current_camera();
}


void Gameworld::update_current_camera() {
	auto& camera = g_game->get_ecs().get_component<Camera>(m_currentCamera);
	auto view = camera.get_transform().to_matrix4x4();

	g_renderer->update_camera(camera.get_transform().get_position(), glm::inverse(view));
}

void Gameworld::set_current_camera(ECS::Entity entity) {
	m_currentCamera = entity;
}

void Gameworld::set_local_player(ECS::Entity entity){
	m_localPlayer = entity;
}

ECS::Entity Gameworld::get_current_camera() const {
	return m_currentCamera;
}

ECS::Entity Gameworld::get_local_player() const {
	return m_localPlayer;
}

void Gameworld::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Gameworld>(entity);
}
