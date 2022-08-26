#include "viewport_gui.hpp"

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <ui/gui_object.hpp>
#include <ui/text_gui_object.hpp>

using namespace Game;

void ViewportGui::set_display_order(ECS::Manager& ecs, Instance& selfInst, int32_t iDisplayOrder) {
	uint32_t displayOrder = static_cast<uint32_t>(iDisplayOrder) + 0x80'00'00'00u;

	if (m_displayOrder == displayOrder) {
		return;
	}

	m_displayOrder = displayOrder;

	mark_for_redraw(ecs, selfInst);
}

void ViewportGui::set_z_index_behavior(ECS::Manager& ecs, Instance& selfInst,
		ZIndexBehavior behavior) {
	if (behavior == m_zIndexBehavior) {
		return;
	}

	m_zIndexBehavior = behavior;

	mark_for_redraw(ecs, selfInst);
}

void ViewportGui::set_enabled(ECS::Manager& ecs, Instance& selfInst, bool enabled) {
	if (enabled == m_enabled) {
		return;
	}

	m_enabled = enabled;

	mark_for_redraw(ecs, selfInst);
}

void ViewportGui::set_ignore_gui_inset(ECS::Manager& ecs, Instance& selfInst, bool ignore) {
	if (m_ignoreGuiInset == ignore) {
		return;
	}

	m_ignoreGuiInset = ignore;

	mark_for_redraw(ecs, selfInst);
}

void ViewportGui::set_absolute_position(Math::Vector2 absolutePosition) {
	m_absolutePosition = std::move(absolutePosition);
}

void ViewportGui::set_absolute_size(Math::Vector2 absoluteSize) {
	m_invSize = 1.f / absoluteSize;
	m_absoluteSize = std::move(absoluteSize);
}

void ViewportGui::mark_for_redraw(ECS::Manager& ecs, Instance& selfInst) {
	m_needsRedraw = true;

	for_each_descendant(ecs, selfInst, [&](auto eDesc, auto& desc) {
		if (auto* uiObj = try_get_gui_object(ecs, eDesc, desc.m_classID); uiObj) {
			//if (Game::TextGuiObject* obj = dynamic_cast<Game::TextGuiObject*>(uiObj))
				//printf("Updating flag for %s\n", obj->get_text().data());
			uiObj->set_update_flag(GuiObject::FLAG_NEEDS_REDRAW
					| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
		}
	});
}

void ViewportGui::clear_redraw() {
	m_needsRedraw = false;
}

const Math::Vector2& ViewportGui::get_absolute_position() const {
	return m_absolutePosition;
}

const Math::Vector2& ViewportGui::get_absolute_size() const {
	return m_absoluteSize;
}

const Math::Vector2& ViewportGui::get_inv_size() const {
	return m_invSize;
}

uint32_t ViewportGui::get_display_order() const {
	return m_displayOrder;
}

ZIndexBehavior ViewportGui::get_z_index_behavior() const {
	return m_zIndexBehavior;
}

bool ViewportGui::is_enabled() const {
	return m_enabled;
}

bool ViewportGui::ignore_gui_inset() const {
	return m_ignoreGuiInset;
}

bool ViewportGui::needs_redraw() const {
	return m_needsRedraw;
}

