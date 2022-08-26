#pragma once

#include <ui/text_gui_object.hpp>
#include <ui/gui_button.hpp>

namespace Game {

class TextButton : public TextGuiObject, public GuiButton {
	public:
		// FIXME: void set_style(ECS::Manager&, Instance&, ButtonStyle);
		
		void set_button_hovered(ECS::Manager&, Instance&);
		void set_button_pressed(ECS::Manager&, Instance&);
		void set_button_normal(ECS::Manager&, Instance&);
};

}

