#include "game/game_camera.hpp"
#include "core/application.hpp"
#include "math/transform.hpp"
#include "ecs/ecs_fwd.hpp"
#include "ecs/ecs.hpp"

#include "GLFW/glfw3.h"
#include "core/data_model.hpp"
#include "game/gameworld.hpp"
#include "core/camera.hpp"
#include "math/matrix4x4.hpp"
#include "player.hpp"
#include "math/vector3.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

using namespace Game;

constexpr float SENSITIVIY = 0.6f;

void Game::update_game_camera([[maybe_unused]] float deltaTime)
{
	static bool lastMouseButtonState = false;
	auto& ecs = g_game->get_ecs();
	auto& workspace = g_game->get_gameworld();
	auto& camera = ecs.get_component<Camera>(workspace.get_current_camera());
	PlayerController& controller = ecs.get_component<PlayerController>(workspace.get_local_player());
	
	Math::Transform offset = {0, 0, 0};
	Math::Transform rotationX;
	Math::Transform rotationY;
	if (g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_RIGHT)) {
	
		glfwSetInputMode(g_window->get_handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		if (lastMouseButtonState)
			glfwSetCursorPos(g_window->get_handle(), static_cast<double>(g_window->get_width()) * 0.5, static_cast<double>(g_window->get_height()) * 0.5);
		lastMouseButtonState = false;
	}
	else
	{
		glfwSetInputMode(g_window->get_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (lastMouseButtonState)
		{
			auto mouseOffset = controller.m_cameraOffset;
			mouseOffset.x -= SENSITIVIY * (float)g_application->get_mouse_delta_x();
			mouseOffset.y -= SENSITIVIY * (float)g_application->get_mouse_delta_y();
			controller.m_cameraOffset = Math::Vector3Lerp(controller.m_cameraOffset, mouseOffset, 0.5f);
		}
		lastMouseButtonState = true;
	}

	rotationX = Math::Transform::from_axis_angle({0, 1, 0}, glm::radians(controller.m_cameraOffset.x)); 
	rotationY = Math::Transform::from_axis_angle({1, 0, 0}, glm::radians(controller.m_cameraOffset.y)); 
	
	offset *= rotationX;
	offset *= rotationY;
	offset *= Math::Transform(0, 0, 1);
	auto cf = Math::Transform(controller.m_transform.get_position());
	camera.set_transform(cf * offset);
}
