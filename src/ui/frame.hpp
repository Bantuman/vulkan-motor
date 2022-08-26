#pragma once

#include <ui/gui_image_emitter.hpp>

namespace Game {

class Rect2D : public GuiImageEmitter {
	public:
		void set_frame_style(ECS::Manager&, Instance& selfInstance, FrameStyle);

		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
				const Math::Rect* clipRect) override;

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);
	private:
		ECS::Entity m_imageQuads[9] = {ECS::INVALID_ENTITY, ECS::INVALID_ENTITY,
				ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY,
				ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY};
		FrameStyle m_frameStyle = FrameStyle::CUSTOM;

		void clear_internal(ECS::Manager&);
		void redraw_internal(ECS::Manager&, RectOrderInfo&, const ViewportGui&,
				const Math::Rect* clipRect);
};

}

