#include "editor_camera.hpp"

#include <core/application.hpp>

#include <ecs/ecs_fwd.hpp>
#include <ecs/ecs.hpp>

#include <core/data_model.hpp>
#include <game/gameworld.hpp>
#include <core/camera.hpp>

using namespace Game;


void Game::update_editor_camera(float deltaTime) {
	auto& ecs = g_game->get_ecs();
	auto& workspace = g_game->get_gameworld();
	auto& camera = ecs.get_component<Camera>(workspace.get_current_camera());

	auto focus = camera.get_focus();
	auto position = camera.get_transform().get_position();

	Math::Vector3 offset{};

	float speed = 40.f;

	if (g_application->is_key_down(GLFW_KEY_LEFT_SHIFT)) {
		speed *= 0.1f;
	}

	if (g_application->is_key_down(GLFW_KEY_W)) {
		offset += camera.get_transform().look_vector() * (speed * deltaTime);
	}

	if (g_application->is_key_down(GLFW_KEY_S)) {
		offset += camera.get_transform().look_vector() * (-speed * deltaTime);
	}

	if (g_application->is_key_down(GLFW_KEY_A)) {
		offset += camera.get_transform().right_vector() * (-speed * deltaTime);
	}

	if (g_application->is_key_down(GLFW_KEY_D)) {
		offset += camera.get_transform().right_vector() * (speed * deltaTime);
	}

	if (g_application->is_key_down(GLFW_KEY_Q)) {
		offset += camera.get_transform().up_vector() * (-speed * deltaTime);
	}

	if (g_application->is_key_down(GLFW_KEY_E)) {
		offset += camera.get_transform().up_vector() * (speed * deltaTime);
	}

	focus += offset;
	position += offset;

	if (g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_RIGHT)) {
		position += camera.get_transform().right_vector()
				* static_cast<float>(g_application->get_mouse_delta_x() * deltaTime * -2.f);
	
		auto newPos = position + camera.get_transform().up_vector()
				* static_cast<float>(g_application->get_mouse_delta_y() * deltaTime * 2.f);
		auto d = Math::normalize(newPos - focus.get_position()).y;

		if (d <= 0.99f && d >= -0.99f) {
			position = std::move(newPos);
		}
	}

	auto cf = Math::Transform::look_at(position, focus.get_position());
	cf.get_position() = focus.get_position()
			+ Math::normalize(position - focus.get_position()) * 2.f;

	camera.set_transform(std::move(cf));
	camera.set_focus(std::move(focus));
}

