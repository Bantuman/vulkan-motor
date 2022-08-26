#include "game/player.hpp"
#include "core/application.hpp"
#include "math/transform.hpp"
#include "ecs/ecs_fwd.hpp"
#include "ecs/ecs.hpp"


void Game::update_player([[maybe_unused]] float deltaTime)
{
}

void Game::update_player_controls([[maybe_unused]] float deltaTime)
{
	/*[[maybe_unused]] auto& ecs = g_game->get_ecs();
	auto& workspace = g_game->get_gameworld();
	
	ECS::Entity player = workspace.get_local_player();
	PlayerController& controller = ecs.get_component<PlayerController>(player);
	auto& camera = ecs.get_component<Camera>(workspace.get_current_camera());
	
	controller.m_acceleration = Math::Vector3(0, 0, 0);
	if (g_application->is_key_down(GLFW_KEY_W))
	{
		controller.m_acceleration += camera.get_transform().look_vector();
	}
	if (g_application->is_key_down(GLFW_KEY_A))
	{
		controller.m_acceleration -= camera.get_transform().right_vector();
	}
	if (g_application->is_key_down(GLFW_KEY_S))
	{
		controller.m_acceleration -= camera.get_transform().look_vector();
	}
	if (g_application->is_key_down(GLFW_KEY_D))
	{
		controller.m_acceleration += camera.get_transform().right_vector();
	}
	if (g_application->is_key_down(GLFW_KEY_E))
	{
		controller.m_roll += deltaTime;
	}
	if (g_application->is_key_down(GLFW_KEY_Q))
	{
		controller.m_roll -= deltaTime;
	}
	if (glm::length(controller.m_acceleration))
	{
		controller.m_acceleration = Math::normalize(controller.m_acceleration);
	}
	printf("roll = %f\n", controller.m_roll);	
	constexpr float ACCELERATION = 250;
	constexpr float MAX_VELOCITY = 500;

	float projvel = glm::dot(controller.m_velocity, controller.m_acceleration);
	float accelvel = ACCELERATION * deltaTime;

	if (projvel + accelvel > MAX_VELOCITY)
	{
		accelvel = MAX_VELOCITY - projvel;
	}

	controller.m_velocity += controller.m_acceleration * accelvel;*/
}
