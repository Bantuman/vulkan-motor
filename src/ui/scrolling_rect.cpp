#include "scrolling_rect.hpp"
#include <core/instance_utils.hpp>
#include <core/application.hpp>
#include <core/hashed_string.hpp>

constexpr float FADE_SPEED = 25.f;
constexpr float SCROLL_SPEED = 10.f;

void Game::ScrollingRect::set_handle_width(float aWidth)
{
	m_width = aWidth;
}

void Game::ScrollingRect::set_direction(ResizeDirection aDirection)
{
	m_direction = aDirection;
}

void Game::ScrollingRect::update(ECS::Manager& ecs, Instance& selfInstance, ECS::Entity self, const Math::Vector2& mousePos, float deltaTime)
{
	float totalHeight = m_absolutePosition.y + m_absoluteSize.y;
	int32_t oldOvershoot = m_overshoot;
	static float oldAbsoluteHeight = 0;
	m_overshoot = 0;
	for_each_child(ecs, selfInstance, [&](auto eDesc, auto desc)
	{
		if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
			const Math::Vector2& uiPos = uiObj->get_position().get_offset();
			const auto& uiSize = uiObj->get_absolute_size();
			if (int32_t absolutey = uiPos.y  + uiSize.y)
			{
				if (int32_t overshoot = absolutey - m_absoluteSize.y; overshoot > m_overshoot)
					m_overshoot = overshoot;
			}
		}
	});
	if (m_overshoot)
	{
		m_height = (totalHeight / (totalHeight + m_overshoot)) * m_absoluteSize.y;
	}
	int32_t maxHeight = static_cast<int32_t>(((m_absoluteSize.y - m_height) * 2));
	if (maxHeight % 2 == 1)
		maxHeight++;

	bool scrolled = false;
	if (double scroll = g_application->get_scroll_delta_y(); abs(scroll) > 0.f && rect_contains_point(mousePos))
	{
		m_offset = std::clamp(static_cast<int32_t>(m_offset - static_cast<float>(scroll * SCROLL_SPEED)), 0, maxHeight);
		m_calculatedOffset = 2 * static_cast<int32_t>((static_cast<float>(m_offset) / static_cast<float>(maxHeight) * m_overshoot));
		scrolled = true;
	}
	if (g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT) && !m_scrolling && handle_contains_point(mousePos))
	{
		m_scrolling = true;
		m_dragAnchor = {0, m_offset};
		m_dragOrigin = mousePos - m_dragAnchor;
	}

	if (!g_application->is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT) && m_scrolling)
	{
		m_scrolling = false;
	}
	if (m_scrolling)
	{
		auto localPos = mousePos - m_dragAnchor;
		auto delta = localPos - m_dragOrigin;

		m_offset = std::clamp(static_cast<int32_t>(m_dragAnchor.y + delta.y), 0, maxHeight);
		m_calculatedOffset = 2 * static_cast<int32_t>((static_cast<float>(m_offset) / static_cast<float>(maxHeight) * m_overshoot));
	}
	Math::Transform2D offset(1.f);
	offset.set_translation({ -m_absoluteSize.x, 0 });
	m_barTransform = m_globalTransform * offset;
	offset.set_translation({ -m_absoluteSize.x, -m_absoluteSize.y + m_height + m_offset});
	m_handleTransform = m_globalTransform * offset;

	if (scrolled || m_scrolling || oldOvershoot != m_overshoot || oldAbsoluteHeight != m_absoluteSize.y)
	{
		mark_for_redraw(ecs, selfInstance);
		//recursive_recalc_position(ecs, selfInstance, self);
		//ecs.add_component<ECS::Tag<"GuiTransformUpdate"_hs>>(self);
		for_each_descendant(ecs, selfInstance, [&](auto eDesc, auto& desc) {
			if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
				uiObj->set_update_flag(GuiObject::FLAG_NEEDS_REDRAW
					| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
			}
		});
	}

	oldAbsoluteHeight = m_absoluteSize.y;
}

int32_t Game::ScrollingRect::get_offset() const
{
	return m_calculatedOffset;
}

void Game::ScrollingRect::clear(ECS::Manager& ecs)
{
	GuiObject::clear(ecs);
	try_remove_rect_raw(ecs, m_handleRect);
	try_remove_rect_raw(ecs, m_barRect);
}

void Game::ScrollingRect::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& viewportGui, const Math::Rect* clipRect)
{
	orderInfo.overlay = false;
	GuiObject::redraw(ecs, orderInfo, viewportGui, clipRect);

	m_barColor = SELECTED_COLOR.to_vector4(0.8f);
	m_handleColor = LIGHT_COLOR.to_vector4(m_overshoot > 0);
	// Emit scrolling bar backdrop
	RectInstance barRect{};
	barRect.color = m_barColor;

	make_screen_transform(barRect.transform, m_barTransform, { m_width, m_absoluteSize.y }, viewportGui.get_inv_size());
	emit_rect(ecs, m_barRect, barRect, Math::Vector2(), orderInfo, nullptr);

	// Emit scrolling bar handle
	RectInstance handleRect{};
	handleRect.color = m_handleColor;
	make_screen_transform(handleRect.transform, m_handleTransform, { m_width, m_height }, viewportGui.get_inv_size());
	orderInfo.overlay = true;
	emit_rect(ecs, m_handleRect, handleRect, Math::Vector2(), orderInfo, nullptr);
}

bool Game::ScrollingRect::handle_contains_point(const Math::Vector2& point) const
{
	auto localPos = m_handleTransform.point_to_local_space(point);
	bool result = Math::abs(localPos.x) <= m_width
		&& Math::abs(localPos.y) <= m_height;
	return result;
}

bool Game::ScrollingRect::bar_contains_point(const Math::Vector2& point) const
{
	auto localPos = m_barTransform.point_to_local_space(point);
	bool result = Math::abs(localPos.x) <= m_width
		&& Math::abs(localPos.y) <= m_absoluteSize.y;
	return result;
}

bool Game::ScrollingRect::rect_contains_point(const Math::Vector2& point) const
{
	auto localPos = m_globalTransform.point_to_local_space(point);
	bool result = Math::abs(localPos.x) <= m_absoluteSize.x
		&& Math::abs(localPos.y) <= m_absoluteSize.y;
	return result;
}

