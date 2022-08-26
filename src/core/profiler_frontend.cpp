#include "profiler_frontend.hpp"

#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/instance_factory.hpp>
#include <core/data_model.hpp>
#include <ui/frame.hpp>

#include <rendering/vk_profiler.hpp>

using namespace Game;

void Game::ProfilerFrontend::init() {
	ECS::Entity eCoreGui;
	g_game->get_singleton(InstanceClass::ENGINE_GUI, eCoreGui);

	ECS::Entity eScreenGui;
	auto* instScreenGui = InstanceFactory::create(*g_ecs, InstanceClass::VIEWPORT_GUI, eScreenGui);
	instScreenGui->m_name = "CoreProfilerGui";

	for (uint32_t i = 0; i < 10; ++i) {
		ECS::Entity eFrame;
		auto* instFrame = InstanceFactory::create(*g_ecs, InstanceClass::RECT, eFrame);
		auto& frame = g_ecs->get_component<Rect2D>(eFrame);

		frame.set_position(*g_ecs, *instFrame, eFrame,
				Math::UDim2{{0.1f * static_cast<float>(i), 0}, {0.f, 0}});
		frame.set_size(*g_ecs, *instFrame, Math::UDim2{{0.1f, 0}, {0.1f, 0}});
		frame.set_background_transparency(*g_ecs, *instFrame, .5f);

		instFrame->set_parent(*g_ecs, eScreenGui, eFrame);
	}

	instScreenGui = &g_ecs->get_component<Instance>(eScreenGui);
	instScreenGui->set_parent(*g_ecs, eCoreGui, eScreenGui);
}

void Game::ProfilerFrontend::deinit() {
}

void Game::ProfilerFrontend::update() {
}

