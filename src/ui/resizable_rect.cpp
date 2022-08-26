#include "resizable_rect.hpp"
#include <core/instance_utils.hpp>
#include <core/application.hpp>

constexpr float FADE_SPEED = 25.f;

void Game::ResizableRect::set_max_size(int16_t aSize)
{
	m_maxSize = aSize;
}

void Game::ResizableRect::set_min_size(int16_t aSize)
{
	m_minSize = aSize;
}

void Game::ResizableRect::set_handle_width(float aWidth)
{
	m_width = aWidth;
}

void Game::ResizableRect::set_direction(ResizeDirection aDirection)
{
	m_direction = aDirection;
}

void Game::ResizableRect::update(ECS::Manager& ecs, Instance& selfInstance, const Math::Vector2& mousePos, float deltaTime)
{
	
	if (m_currentSize < 0)
	{
		m_currentSize = m_absoluteSize.x;
	}
	
	bool dragging = is_mouse_inside();
	m_handleColor = LIGHT_COLOR.to_vector4(m_handleColor.w + ((dragging || m_dragging) - m_handleColor.w) * (deltaTime * FADE_SPEED));

	if (!m_dragging && dragging && g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT))
	{
		m_dragging = true;
		m_dragDelta = get_absolute_size();
		m_dragAnchor = get_absolute_position();
		m_dragOrigin = mousePos - m_dragAnchor;
	}
	if (m_dragging && !g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT))
	{
		m_dragging = false;
		for_each_descendant(ecs, selfInstance, [&](auto eDesc, auto& desc) {
			if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
				uiObj->set_update_flag(GuiObject::FLAG_NEEDS_REDRAW
					| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
			}
		});

	}
	if (m_dragging)
	{
		auto localPos = mousePos - m_dragAnchor;
		auto delta = localPos - m_dragOrigin;
		bool direction = (m_dragDelta + delta).x > m_currentSize;
		m_currentSize = (m_dragDelta + delta).x;
		m_currentSize = std::clamp(m_currentSize, m_minSize, m_maxSize);
		auto& size = get_size();
		set_size(ecs, selfInstance, { size.x.scale, m_currentSize, size.y.scale, size.y.offset });
		if (direction)
		{
			for_each_child(ecs, selfInstance, [&](auto eDesc, auto& desc) {
				if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
					uiObj->set_update_flag(GuiObject::FLAG_NEEDS_REDRAW
						| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
				}
			});
		}
		else
		{
			for_each_descendant(ecs, selfInstance, [&](auto eDesc, auto& desc) {
				if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
					uiObj->set_update_flag(GuiObject::FLAG_NEEDS_REDRAW
						| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
				}
			});
		}
	}

	Math::Transform2D offset(1.f);
	offset.set_translation({ m_currentSize + m_width, 0 });
	m_handleTransform = m_globalTransform * offset;
	mark_for_redraw(ecs, selfInstance);
}

void Game::ResizableRect::clear(ECS::Manager& ecs)
{
	GuiObject::clear(ecs);
}

void Game::ResizableRect::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& viewportGui, const Math::Rect* clipRect)
{
	GuiObject::redraw(ecs, orderInfo, viewportGui, clipRect);

	// Emit resizable bar rect
	RectInstance rect{};
	rect.color = m_handleColor;

	make_screen_transform(rect.transform, m_handleTransform, { m_width, m_absoluteSize.y }, viewportGui.get_inv_size());
	orderInfo.overlay = true;
	emit_rect(ecs, m_handleRect, rect, Math::Vector2(), orderInfo, nullptr);
}

bool Game::ResizableRect::contains_point(const Math::Vector2& point, const Math::Vector2& invScreen) const
{
	auto localPos = m_handleTransform.point_to_local_space(point);
	bool result = Math::abs(localPos.x) <= m_width
		&& Math::abs(localPos.y) <= m_absoluteSize.y;
	return result;
}
