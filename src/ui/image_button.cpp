#include "image_button.hpp"

#include <ecs/ecs.hpp>

using namespace Game;

void ImageButton::set_hover_image(ECS::Manager& ecs, Instance& inst,
		const std::string& hoverImage) {
	if (m_hoverImage.compare(hoverImage) != 0) {
		m_hoverImage = hoverImage;
		mark_for_redraw(ecs, inst);
	}
}

void ImageButton::set_pressed_image(ECS::Manager& ecs, Instance& inst,
		const std::string& pressedImage) {
	if (m_pressedImage.compare(pressedImage) != 0) {
		m_pressedImage = pressedImage;
		mark_for_redraw(ecs, inst);
	}
}

void ImageButton::set_button_hovered(ECS::Manager& ecs, Instance& inst) {
	if (is_visible() && is_active()) {
		if (has_auto_button_color()) {
			set_background_color(ecs, get_background_color3() * 0.7f);
		}
		else if (m_hoverImage.size() != 0) { // FIXME: better check for the existence of this texture
			mark_for_redraw(ecs, inst);
		}
	}
}

void ImageButton::set_button_pressed(ECS::Manager& ecs, Instance& inst) {
	if (is_visible() && is_active()) {
		if (has_auto_button_color()) {
			set_background_color(ecs, get_background_color3() * 0.7f);
		}
		else if (m_pressedImage.size() != 0) { // FIXME: better check for the existence of this texture
			mark_for_redraw(ecs, inst);
		}
	}
}

void ImageButton::set_button_normal(ECS::Manager& ecs, Instance& inst) {
	if (is_visible() && is_active()) {
		if (has_auto_button_color()) {
			set_background_color(ecs, get_background_color3());
		}
		else if (m_hoverImage.size() != 0 || m_pressedImage.size() != 0) { // FIXME: better check for the existence of this texture
			mark_for_redraw(ecs, inst);
		}
	}
}

