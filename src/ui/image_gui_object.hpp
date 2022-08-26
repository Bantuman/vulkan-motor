#pragma once

#include <string>

#include <ui/gui_image_emitter.hpp>

namespace Game {

class ImageGuiObject : public GuiImageEmitter {
	public:
		ECS::Entity m_imageQuads[9] = {ECS::INVALID_ENTITY, ECS::INVALID_ENTITY,
				ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY,
				ECS::INVALID_ENTITY, ECS::INVALID_ENTITY, ECS::INVALID_ENTITY};

		std::string m_image;
		Math::Color3 m_imageColor3;
		Math::Vector2 m_imageRectOffset;
		Math::Vector2 m_imageRectSize;
		float m_imageTransparency;
		// bool m_isLoaded;
		ResamplerMode m_resampleMode;
		ScaleType m_scaleType;
		Math::Rect m_sliceCenter;
		float m_sliceScale;
		Math::UDim2 m_tileSize;

		// METHODS
		// FIXME: maybe also a string&& version?
		void set_image(ECS::Manager&, Instance& selfInstance, const std::string&);

		void set_image_color3(ECS::Manager&, const Math::Color3&);
		void set_image_rect_offset(ECS::Manager&, Instance& selfInstance, const Math::Vector2&);
		void set_image_rect_size(ECS::Manager&, Instance& selfInstance, const Math::Vector2&);
		void set_image_transparency(ECS::Manager&, Instance& selfInstance, float);
		void set_resample_mode(ECS::Manager&, ResamplerMode);
		void set_scale_type(ECS::Manager&, Instance& selfInstance, ScaleType);
		void set_slice_center(ECS::Manager&, Instance& selfInstance, const Math::Rect&);
		void set_slice_scale(ECS::Manager&, Instance& selfInstance, float);
		void set_tile_size(ECS::Manager&, Instance& selfInstance, const Math::UDim2&);

		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
				const Math::Rect* clipRect) override;

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);
	private:
		void clear_internal(ECS::Manager&);
		void redraw_internal(ECS::Manager&, RectOrderInfo&, const ViewportGui&,
				const Math::Rect* clipRect);
};

}

