#pragma once
#include <ui/gui_image_emitter.hpp>

using namespace Game;

namespace Game {

	class ScrollingRect : public GuiImageEmitter {
	public:
		void set_handle_width(float aWidth);
		void set_direction(ResizeDirection aDirection);

		void update(ECS::Manager&, Instance& selfInstance, ECS::Entity self, const Math::Vector2& mousePos, float deltaTime);
		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&, const Math::Rect* clipRect) override;

		bool bar_contains_point(const Math::Vector2& point) const;
		bool handle_contains_point(const Math::Vector2& point) const;
		bool rect_contains_point(const Math::Vector2& point) const;
		int32_t get_offset() const;

	private:
		bool m_scrolling;
		float m_width;
		float m_height = 64;
		int32_t m_overshoot;
		int32_t m_calculatedOffset;
		int32_t m_offset;
		Math::Vector4 m_handleColor;
		Math::Transform2D m_handleTransform;
		Math::Vector4 m_barColor;
		Math::Transform2D m_barTransform;

		Math::Vector2 m_dragAnchor;
		Math::Vector2 m_dragDelta;
		Math::Vector2 m_dragOrigin;
		ResizeDirection m_direction;
		ECS::Entity m_handleRect = ECS::INVALID_ENTITY;
		ECS::Entity m_barRect = ECS::INVALID_ENTITY;
	};

}

