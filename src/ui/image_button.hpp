#pragma once

#include <ui/image_gui_object.hpp>
#include <ui/gui_button.hpp>

namespace Game {

class ImageButton : public ImageGuiObject, public GuiButton {
	public:
		void set_hover_image(ECS::Manager&, Instance& selfInstance, const std::string&);
		void set_pressed_image(ECS::Manager&, Instance& selfInstance, const std::string&);

		// FIXME: void set_style(ECS::Manager&, Instance&, ButtonStyle);
		
		void set_button_hovered(ECS::Manager&, Instance& selfInstance);
		void set_button_pressed(ECS::Manager&, Instance& selfInstance);
		void set_button_normal(ECS::Manager&, Instance& selfInstance);
	private:
		std::string m_hoverImage;
		std::string m_pressedImage;
};

}

