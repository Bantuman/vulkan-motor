#pragma once

#include <ecs/ecs_fwd.hpp>
#include <functional>
#include <ui/ui_enumerations.hpp>

namespace Game {

class GuiButton {
	public:
		void set_auto_button_color(bool);
		void set_modal(bool);
		void set_callback(std::function<void(ECS::Entity)>&&);
		std::function<void(ECS::Entity)> get_callback();

		bool has_auto_button_color() const;
		bool is_modal() const;
	protected:
		ButtonStyle m_style;
		bool m_autoButtonColor;
		bool m_modal;
		std::function<void(ECS::Entity)> m_callback = nullptr;
};

}

