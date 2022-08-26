#include "ecs/ecs_fwd.hpp"
#include "game/game_camera.hpp"
#include "core/instance_class.hpp"
#include "core/instance_factory.hpp"
#include "core/mesh_geom.hpp"
#include "core/model.hpp"

#include "asset/scene_loader.hpp"

#include <cstdio>
#include <random>
#include <animation/rig.hpp>

#include <asset/geom_mesh_cache.hpp>
#include <asset/rigged_mesh_cache.hpp>
#include <asset/texture_cache.hpp>
#include <asset/cube_map_cache.hpp>
#include <asset/font_cache.hpp>
#include <asset/font_family_cache.hpp>
#include <asset/animation_cache.hpp>

#include <core/application.hpp>
#include <core/hashed_string.hpp>
#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <file/file_system.hpp>

#include <core/components.hpp>
#include <core/context_action.hpp>
#include <rendering/renderer/game_renderer.hpp>
#include <core/instance_utils.hpp>
#include <core/ancestry_changed_callbacks.hpp>
#include <core/destroyed_callbacks.hpp>
#include <asset/rigged_mesh_loader.hpp>
#include <animation/rig_component.hpp>
#include <core/profiler_frontend.hpp>
#include <editor/editor_ui.hpp>
#include <editor/editor_camera.hpp>
#include "core/data_model.hpp"

#include <ui/ui_components.hpp>
#include <ui/ui_render_components.hpp>
#include <ui/game_ui.hpp>

#include <math/common.hpp>
#include <math/trigonometric.hpp>

#include <net/web.hpp>

#include <rendering/vk_profiler.hpp>
#include <string_view>

#ifdef _DEBUG
bool EDITOR_DEBUG = true;
#pragma comment(lib,"Engine_Debug.lib")
#endif // DEBUG
#ifdef _RELEASE
bool EDITOR_DEBUG = false;
#pragma comment(lib,"Engine_Release.lib")
#endif // _RELEASE
#ifdef _RETAIL
bool EDITOR_DEBUG = false;
#pragma comment(lib,"Engine_Retail.lib")
#endif // _RETAIL

using namespace Game;

int main(int argc, char** argv) {
	std::string_view gameFilePath = "Z:\\Archive\\somefile.something";

	const std::vector<std::string_view> args(argv + 1, argv + argc);
	for (const auto& arg : args)
	{
		if (arg == "-d" || arg == "--debug")
		{
			EDITOR_DEBUG = true;			
		}
		else
		{
			gameFilePath = arg;
		}
	}

	Web::init();

	g_fileSystem.create();

	g_geomMeshCache.create();
	g_riggedMeshCache.create();
	g_textureCache.create();
	g_cubeMapCache.create();
	g_fontCache.create();
	g_fontFamilyCache.create();
	g_animationCache.create();

	g_ecs.create();

	g_contextActionManager.create();
	g_application.create();
	//Window window(800, 600, "Vulkan Engine");
	g_window.create(1263, 662, "Vulkan Engine");
	g_renderContext.create();
	g_renderer.create(*g_renderContext);
	Game::init_ancestry_changed_callbacks();
	Game::init_destroyed_callbacks();
	Game::init_ui(*g_ecs);

	g_game = std::make_shared<Game::DataModel>(*g_ecs);
	ECS::Entity eGameworld;
	ECS::Entity eSkybox;
	ECS::Entity eAmbience;
	InstanceFactory::create(*g_ecs, InstanceClass::GAMEWORLD, eGameworld);
	InstanceFactory::create(*g_ecs, InstanceClass::SKY, eSkybox);

	auto& skybox = g_ecs->get_component<Game::Sky>(eSkybox);
	const char* background = "res://space.png";//"res://1362082663558.png";
	skybox.set_back_id(background);
	skybox.set_front_id(background);
	skybox.set_up_id(background);
	skybox.set_down_id(background);
	skybox.set_left_id(background);
	skybox.set_right_id(background);
	
	auto& gameworld = g_ecs->get_component<Game::Gameworld>(eGameworld);
	gameworld.init();

	g_game->get_singleton(InstanceClass::AMBIENCE, eAmbience);

	g_ecs->get_component<Ambience>(eAmbience).set_active_skybox(eSkybox, &skybox);

	g_game->set_gameworld(eGameworld);




	//if (!Game::RiggedMeshLoader::load_scene(*g_ecs, "res://ranka_lee.glb", true)) {
	//	LOG_TEMP2("Failed to load rigged mesh");
	//}

	//
	//for (auto& [key, _] : *g_animationCache) {
	//	LOG_TEMP("animation: %s", key.c_str());
	//}
	//{
	//	auto eWorkspace = ECS::INVALID_ENTITY;
	//	auto* instWorkspace = Game::g_game->get_singleton(Game::InstanceClass::GAMEWORLD,
	//		eWorkspace);
	//	
	//	auto eImportedRig = instWorkspace->find_first_child(*g_ecs, "ImportedRig");
	//	auto& instImportedRig = g_ecs->get_component<Game::Instance>(eImportedRig);
	//	auto& modelImportedRig = g_ecs->get_component<Game::Model>(eImportedRig);
	//	g_ecs->add_component<ECS::Tag<"CharacterMesh"_hs>>(eImportedRig);
	//	
	//	modelImportedRig.set_primary_geom_transform(*g_ecs, instImportedRig, Math::Transform(0, 5, 0));

	//	auto eAnimator = instImportedRig.find_first_child(*g_ecs, "AnimationController");
	//	auto& animator = g_ecs->get_component<Game::Animator>(eAnimator);

	//	animator.m_currentAnim = g_animationCache->get("95 frames_bone");

	//	g_textureCache->get_or_load<TextureLoader>("RankaLeeBody", *g_renderContext,
	//			"res://diva_006_cos_002_body_main_tex_col.png", false, true);
	//	g_textureCache->get_or_load<TextureLoader>("RankaLeeEyes", *g_renderContext,
	//			"res://diva_006_cos_002_body_eye_tex_base.png", false, true);

	//	g_textureCache->get_or_load<TextureLoader>("MMDTestBody", *g_renderContext,
	//			"res://TEX/body.png", true, true);
	//	g_textureCache->get_or_load<TextureLoader>("MMDTestEyes", *g_renderContext,
	//			"res://TEX/EYE.png", true, true);
	//	g_textureCache->get_or_load<TextureLoader>("MMDTestHair", *g_renderContext,
	//			"res://TEX/HAIR.png", true, true);
	//	g_textureCache->get_or_load<TextureLoader>("MMDTestClothes1", *g_renderContext,
	//			"res://TEX/C1-1.png", true, true);
	//	g_textureCache->get_or_load<TextureLoader>("MMDTestClothes2", *g_renderContext,
	//			"res://TEX/C1-2.png", true, true);

	//	if (animator.m_currentAnim) {
	//		LOG_TEMP2("Was successfully able to load the anim");
	//	}

	//	for_each_descendant(*g_ecs, instImportedRig, [&](auto entity, auto& desc) {
	//		if (desc.m_classID == Game::InstanceClass::MESH_GEOM) {
	//			auto& mp = g_ecs->get_component<Game::MeshGeom>(entity);
	//			mp.set_size(entity, Math::Vector3(10, 10, 10));

	//			if (desc.m_name.compare("Ranka Lee_Body") == 0
	//					|| desc.m_name.compare("Ranka Lee_Skirt") == 0) {
	//				mp.set_texture(entity, "RankaLeeBody");
	//			}
	//			else if (desc.m_name.compare("Ranka Lee_Eyes") == 0) {
	//				mp.set_texture(entity, "RankaLeeEyes");
	//			}
	//			else if (desc.m_name.compare("MMDTest_Body") == 0
	//					|| desc.m_name.compare("MMDTest_EyeWhites") == 0) {
	//				mp.set_texture(entity, "MMDTestBody");
	//			}
	//			else if (desc.m_name.compare("MMDTest_Hair") == 0) {
	//				mp.set_texture(entity, "MMDTestHair");
	//			}
	//			else if (desc.m_name.compare("MMDTest_Eyes") == 0) {
	//				mp.set_texture(entity, "MMDTestEyes");
	//			}
	//			else if (desc.m_name.compare("MMDTest_ArmTransparent") == 0
	//					|| desc.m_name.compare("MMDTest_Clothes2") == 0) {
	//				mp.set_texture(entity, "MMDTestClothes1");
	//			}
	//			else if (desc.m_name.compare("MMDTest_GoldRings1") == 0
	//					|| desc.m_name.compare("MMDTest_Clothes1") == 0
	//					|| desc.m_name.compare("MMDTest_GoldRings2") == 0) {
	//				mp.set_texture(entity, "MMDTestClothes2");
	//			}
	//		}
	//	
	//	});
	//
	//}

	g_application->key_down_event().connect([&](auto keyCode) {
		if (keyCode == GLFW_KEY_F) {
			ECS::Entity eStarterGui;
			auto* instStarterGui = Game::g_game->get_singleton(Game::InstanceClass::GAME_GUI,
					eStarterGui);

			Game::for_each_child(*g_ecs, *instStarterGui, [&](auto eChild, auto& child) {
				if (child.m_classID == Game::InstanceClass::VIEWPORT_GUI) {
					auto& screenGui = g_ecs->get_component<Game::ViewportGui>(eChild);
					screenGui.set_enabled(*g_ecs, child, !screenGui.is_enabled());
				}
			});
		}
		else if (keyCode == GLFW_KEY_G) {
			ECS::Entity eCoreGui;
			auto* instCoreGui = Game::g_game->get_singleton(Game::InstanceClass::ENGINE_GUI, eCoreGui);

			Game::for_each_child(*g_ecs, *instCoreGui, [&](auto eChild, auto& child) {
				auto& screenGui = g_ecs->get_component<Game::ViewportGui>(eChild);
				screenGui.set_enabled(*g_ecs, child, !screenGui.is_enabled());
			});
		}
	});

//	Game::ProfilerFrontend::init();
#ifdef _DEBUG
	Game::EditorFrontend::init();
#endif
	double lastTime = g_application->get_time();

	while (!g_window->is_close_requested()) {
		double currTime = g_application->get_time();
		float deltaTime = static_cast<float>(currTime - lastTime);
		lastTime = currTime;

		g_application->poll_events();

	//	Game::ProfilerFrontend::update();
#ifdef _DEBUG
		Game::EditorFrontend::update();
#endif
		Game::update_ui(*g_ecs, deltaTime);
		Game::update_animators(*g_ecs, deltaTime);
		Game::update_rigs(*g_ecs);

		gameworld.update(deltaTime);
		if (EDITOR_DEBUG)
		{
#ifdef _DEBUG
			Game::update_editor_camera(deltaTime);
#endif
		}
		else
		{
			Game::update_game_camera(deltaTime);
		}

		g_renderer->render();
		g_window->swap_buffers();

		g_application->reset_deltas();
		/*LOG_TEMP2("FRAME INFO BEGIN -----------------------------------------");

		for (auto& [k, v] : g_vulkanProfiler->get_timing_data()) {
			LOG_TEMP("TIME %s %f ms", k.c_str(), v);
		}

		for (auto& [k, v] : g_vulkanProfiler->get_stat_data()) {
			LOG_TEMP("STAT %s %d ms", k.c_str(), v);
		}

		LOG_TEMP2("FRAME INFO END -------------------------------------------");*/
	}

	Game::ProfilerFrontend::deinit();

	g_renderer.destroy();

	g_ecs.destroy();

	g_animationCache.destroy();
	g_fontFamilyCache.destroy();
	g_fontCache.destroy();
	g_cubeMapCache.destroy();
	g_textureCache.destroy();
	g_riggedMeshCache.destroy();
	g_geomMeshCache.destroy();

	g_renderContext.destroy();
	g_application.destroy();
	g_contextActionManager.destroy();

	g_fileSystem.destroy();
	
	Web::deinit();
}

