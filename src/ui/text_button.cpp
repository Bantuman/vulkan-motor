#include "text_button.hpp"

#include <ecs/ecs.hpp>

using namespace Game;

void TextButton::set_button_hovered(ECS::Manager& ecs, Instance&) {
	if (is_visible() && is_active() && has_auto_button_color()) {
		set_background_color(ecs, get_background_color3() * 0.7f);
	}
}

void TextButton::set_button_pressed(ECS::Manager& ecs, Instance& inst) {
	set_button_hovered(ecs, inst);
}

void TextButton::set_button_normal(ECS::Manager& ecs, Instance&) {
	if (is_visible() && is_active() && has_auto_button_color()) {
		set_background_color(ecs, get_background_color3());
	}
}

