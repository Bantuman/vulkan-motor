#include "gui_button.hpp"

using namespace Game;

void GuiButton::set_auto_button_color(bool abc) {
	m_autoButtonColor = abc;
}

void GuiButton::set_modal(bool modal) {
	m_modal = modal;
}

void Game::GuiButton::set_callback(std::function<void(ECS::Entity)>&& aCallback)
{
	m_callback = aCallback;
}

std::function<void(ECS::Entity)> Game::GuiButton::get_callback()
{
	if (m_callback)
		return m_callback;
	return nullptr;
}

bool GuiButton::has_auto_button_color() const {
	return m_autoButtonColor;
}

bool GuiButton::is_modal() const {
	return m_modal;
}

