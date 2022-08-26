#pragma once

#include <ui/gui_image_emitter.hpp>

namespace Game {

	class ResizableRect : public GuiImageEmitter {
	public:
		void set_max_size(int16_t aSize);
		void set_min_size(int16_t aSize);
		void set_handle_width(float aWidth);
		void set_direction(ResizeDirection aDirection);

		void update(ECS::Manager&, Instance& selfInstance, const Math::Vector2& mousePos,  float deltaTime);
		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&, const Math::Rect* clipRect) override;
		bool contains_point(const Math::Vector2& point, const Math::Vector2& invScreen) const override;

	private:
		bool m_dragging;
		float m_width;
		int16_t m_maxSize;
		int16_t m_minSize;
		int16_t m_currentSize = -1;
		Math::Vector4 m_handleColor;
		Math::Transform2D m_handleTransform;
		Math::Vector2 m_dragAnchor;
		Math::Vector2 m_dragDelta;
		Math::Vector2 m_dragOrigin;
		ResizeDirection m_direction;
		ECS::Entity m_handleRect = ECS::INVALID_ENTITY;
	};

}

