#pragma once

#include <ui/gui_object.hpp>

class Texture;

namespace Game {

class GuiImageEmitter : public GuiObject {
	public:
	protected:
		void draw_normal_image(ECS::Manager&, ECS::Entity& imgEntity,
				const Math::Vector2& invScreen, const RectOrderInfo& orderInfo,
				Texture&, const Math::Rect* clipRect, const Math::Color3& imageColor,
				float imageTransparency, ResamplerMode, ScaleType, const Math::UDim2& tileSize,
				const Math::Vector2& imageRectSize, const Math::Vector2& imageRectOffset);
		void draw_9slice_image(ECS::Manager&, ECS::Entity* imgEntities,
				const Math::Vector2& invScreen, const RectOrderInfo& orderInfo,
				Texture&, const Math::Rect* clipRect, const Math::Color3& imageColor,
				float imageTransparency, ResamplerMode, const Math::Rect& sliceCenter,
				float sliceScale);
};

}
