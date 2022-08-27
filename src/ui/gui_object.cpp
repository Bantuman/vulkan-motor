#include "ui_components.hpp"

#include <core/logging.hpp>
#include <core/hashed_string.hpp>

#include <ecs/ecs.hpp>

#include <core/instance_utils.hpp>
#include <ui/ui_render_components.hpp>
#include <rendering/renderer/ui_renderer.hpp>

#include <math/common.hpp>

using namespace Game;

static void update_child_position(ECS::Manager& ecs, ECS::Entity entity, Instance& descInst,
		GuiObject*& lastParent, const Math::Vector2& screenSize,
		const Math::Vector2& invScreenSize, const Math::Rect* clipRect, ECS::Entity clipEntity) {
	if (descInst.is_a(InstanceClass::GUI_OBJECT)) {
		auto& uiObj = *try_get_gui_object(ecs, entity, descInst.m_classID);

		auto transform = lastParent->get_global_transform();
		
		uiObj.recalc_position(transform, lastParent->get_absolute_size(),
				lastParent->get_absolute_rotation(), screenSize);

		if (uiObj.get_clip_source() != ECS::INVALID_ENTITY) {
			uiObj.mark_for_redraw(ecs, descInst);
		}
		else {
			ecs.get_or_add_component<ECS::Tag<"GuiTransformUpdate"_hs>>(entity);
		}

		uiObj.recalc_clip_rect(invScreenSize, clipRect, clipEntity);

		lastParent = &uiObj;
	}
}

void GuiObject::set_position(ECS::Manager& ecs, Instance& selfInst, ECS::Entity self,
		const Math::UDim2& position) {
	m_position = position;
	recursive_recalc_position(ecs, selfInst, self);
}

void GuiObject::set_rotation(ECS::Manager& ecs, Instance& selfInst, ECS::Entity self,
		float rotation) {
	if (rotation != m_rotation) {
		m_rotation = rotation;
		recursive_recalc_position(ecs, selfInst, self);
	}
}

void GuiObject::set_anchor_point(ECS::Manager& ecs, Instance& selfInst, ECS::Entity self,
		const Math::Vector2& anchorPoint) {
	m_anchorPoint = anchorPoint;
	recursive_recalc_position(ecs, selfInst, self);
}

const Math::UDim2& GuiObject::get_size() const
{
	return m_size;
}

const Math::UDim2& Game::GuiObject::get_position() const
{
	return m_position;
}

void GuiObject::set_size(ECS::Manager& ecs, Instance& inst, const Math::UDim2& size) {
	m_size = size;
	mark_for_redraw(ecs, inst);
}

void GuiObject::set_background_color3(ECS::Manager& ecs, const Math::Color3& color) {
	m_backgroundColor3 = color;

	if (m_visible) {
		set_background_color(ecs, color);
	}
}

void GuiObject::set_border_color3(ECS::Manager& ecs, const Math::Color3& color) {
	m_borderColor3 = color;

	if (m_visible) {
		auto color4 = color.to_vector4(1.f - m_backgroundTransparency);

		for (size_t i = 0; i < 4; ++i) {
			try_set_rect_color(ecs, m_borderRects[i], color4);
		}
	}
}

void GuiObject::set_background_transparency(ECS::Manager& ecs, Instance& inst,
		float transparency) {
	if (transparency == m_backgroundTransparency) {
		return;
	}

	if (transparency == 1.f || m_backgroundTransparency == 1.f) {
		m_backgroundTransparency = transparency;
		mark_for_redraw(ecs, inst);
	}
	else {
		m_backgroundTransparency = transparency;

		try_set_rect_color(ecs, m_backgroundRect,
				m_backgroundColor3.to_vector4(1.f - transparency));

		auto borderColor = m_borderColor3.to_vector4(1.f - transparency);

		for (size_t i = 0; i < 4; ++i) {
			try_set_rect_color(ecs, m_borderRects[i], borderColor);
		}
	}
}

void GuiObject::set_border_size_pixel(ECS::Manager& ecs, Instance& selfInstance,
		int32_t borderSizePixel) {
	borderSizePixel = borderSizePixel < 0 ? 0 : borderSizePixel;

	if (borderSizePixel != m_borderSizePixel) {
		m_borderSizePixel = borderSizePixel;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_z_index(ECS::Manager& ecs, Instance& selfInstance, int32_t zIndex) {
	auto uZIndex = static_cast<uint32_t>(zIndex) + 0x80'00'00'00u;
	

	if (uZIndex != m_zIndex) {
		m_zIndex = uZIndex;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_layout_order(ECS::Manager& ecs, Instance& selfInstance, int32_t layoutOrder) {
	if (layoutOrder != m_layoutOrder) {
		m_layoutOrder = layoutOrder;
		mark_for_redraw(ecs, selfInstance); // FIXME: I think this is mostly untrue
	}
}

void GuiObject::set_size_constraint(ECS::Manager& ecs, Instance& selfInstance, SizeConstraint sc) {
	if (sc != m_sizeConstraint) {
		m_sizeConstraint = sc;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_border_mode(ECS::Manager& ecs, Instance& selfInstance, BorderMode bm) {
	if (bm != m_borderMode) {
		m_borderMode = bm;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_automatic_size(ECS::Manager& ecs, Instance& selfInstance, AutomaticSize as) {
	if (as != m_automaticSize) {
		m_automaticSize = as;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_visible(ECS::Manager& ecs, Instance& selfInstance, bool visible) {
	if (visible != m_visible) {
		m_visible = visible;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_clips_descendants(ECS::Manager& ecs, Instance& selfInstance,
		bool clipsDescendants) {
	if (clipsDescendants != m_clipsDescendants) {
		m_clipsDescendants = clipsDescendants;
		mark_for_redraw(ecs, selfInstance);
	}
}

void GuiObject::set_active(bool active) {
	m_active = active;
}

void GuiObject::set_mouse_inside(bool inside) {
	m_mouseInside = inside;
}

const Math::Vector2& GuiObject::get_absolute_position() const {
	return m_absolutePosition;
}

const Math::Vector2& GuiObject::get_absolute_size() const {
	return m_absoluteSize;
}

float GuiObject::get_absolute_rotation() const {
	return m_absoluteRotation;
}

const Math::Transform2D& GuiObject::get_global_transform() const {
	return m_globalTransform;
}

const Math::Rect& GuiObject::get_clipped_rect() const {
	return m_clippedRect;
}

ECS::Entity GuiObject::get_clip_source() const {
	return m_clipSource;
}

uint32_t GuiObject::get_z_index() const {
	return m_zIndex;
}

const Math::Color3& GuiObject::get_background_color3() const {
	return m_backgroundColor3;
}

const RectPriority& GuiObject::get_priority() const {
	return m_priority;
}

bool GuiObject::is_visible() const {
	return m_visible;
}

bool GuiObject::clips_descendants() const {
	return m_clipsDescendants;
}

bool GuiObject::is_active() const {
	return m_active;
}

bool GuiObject::is_mouse_inside() const {
	return m_mouseInside;
}

bool GuiObject::contains_point(const Math::Vector2& point, const Math::Vector2& invScreen) const {

	auto localPos = m_globalTransform.point_to_local_space(point);
	bool result = Math::abs(localPos.x) <= m_absoluteSize.x
			&& Math::abs(localPos.y) <= m_absoluteSize.y;

	if (!(m_absoluteSize.x > 0 && m_absoluteSize.y > 0))
	{
		return false;
	}
	if (result && m_clipSource != ECS::INVALID_ENTITY) {
		return m_clippedRect.contains(point * invScreen);
	}

	return result;
}

void GuiObject::mark_for_redraw(ECS::Manager& ecs, Instance& inst) {
	set_update_flag(FLAG_NEEDS_REDRAW);

	for_each_ancestor(ecs, inst, [&](auto entity, auto& parent) {
		if (parent.is_a(InstanceClass::GUI_OBJECT)) {
			auto& uiObj = *try_get_gui_object(ecs, entity, parent.m_classID);
			uiObj.set_update_flag(FLAG_CHILD_NEEDS_REDRAW);
		}
		else if (parent.is_a(InstanceClass::VIEWPORT_GUI)) {
			ecs.get_component<ViewportGui>(entity).m_needsRedraw = true;
		}
	});
}

void GuiObject::clear(ECS::Manager& ecs) {
	try_remove_rect_raw(ecs, m_backgroundRect);

	for (size_t i = 0; i < 4; ++i) {
		try_remove_rect_raw(ecs, m_borderRects[i]);
	}
}

void GuiObject::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& screenGui,
		const Math::Rect* clipRect) {
	m_priority = RectPriority(orderInfo);

	if (m_backgroundTransparency == 1.f) {
		GuiObject::clear(ecs);
		return;
	}

	orderInfo.zIndex = m_zIndex;
	orderInfo.overlay = false;

	auto drawSize = m_absoluteSize;

	switch (m_borderMode) {
		case BorderMode::MIDDLE:
			drawSize -= Math::Vector2(static_cast<float>(m_borderSizePixel));
			break;
		case BorderMode::INSET:
			drawSize -= Math::Vector2(2.f * static_cast<float>(m_borderSizePixel));
			break;
		default:
			break;
	}

	RectInstance rect{};

	make_screen_transform(rect.transform, m_globalTransform, drawSize, screenGui.m_invSize);
	rect.color = m_backgroundColor3.to_vector4(1.f - m_backgroundTransparency);
	m_priority = RectPriority(orderInfo);
	emit_rect(ecs, m_backgroundRect, rect, Math::Vector2(), orderInfo, clipRect);

	if (m_borderSizePixel > 0) {
		draw_borders(ecs, screenGui.m_invSize, orderInfo, clipRect);
	}
}

void GuiObject::recursive_clear(ECS::Manager& ecs, Instance& inst) {
	clear(ecs);

	for_each_descendant(ecs, inst, [&](auto entity, auto& child) {
		if (child.is_a(InstanceClass::GUI_OBJECT)) {
			auto& uiObj = *try_get_gui_object(ecs, entity, child.m_classID);
			uiObj.clear(ecs);
			uiObj.clear_update_flag(FLAG_NEEDS_REDRAW | FLAG_CHILD_NEEDS_REDRAW);
		}
	});

	clear_update_flag(FLAG_NEEDS_REDRAW | FLAG_CHILD_NEEDS_REDRAW);
}

void GuiObject::recalc_size(const Math::Vector2& parentScale) {
	m_absoluteSize = m_size.get_offset();

	switch (m_sizeConstraint) {
		case SizeConstraint::RELATIVE_XY:
			m_absoluteSize += m_size.get_scale() * parentScale;
			break;
		case SizeConstraint::RELATIVE_XX:
			m_absoluteSize += Math::Vector2(m_size.x.scale, m_size.x.scale) * parentScale;
			break;
		case SizeConstraint::RELATIVE_YY:
			m_absoluteSize += Math::Vector2(m_size.y.scale, m_size.y.scale) * parentScale;
			break;
	}
}

void GuiObject::recalc_position(const Math::Transform2D& parentTransform,
		const Math::Vector2& parentScale, float parentRotation, const Math::Vector2& screenSize) {
	auto localPosition = m_position.get_scale() * parentScale
			+ m_position.get_offset() - (m_absoluteSize * m_anchorPoint)
			+ 0.5f * m_absoluteSize;

	auto localTransform = Math::Transform2D()
			.set_rotation(Math::radians(m_rotation))
			.set_translation(2.f * localPosition - parentScale);

	m_globalTransform = parentTransform * localTransform;
	m_absolutePosition = (m_globalTransform[2] - m_absoluteSize + screenSize) * 0.5f;
	m_absoluteRotation = parentRotation + m_rotation;
}

bool GuiObject::recalc_clip_rect(const Math::Vector2& invScreenSize, const Math::Rect* clipRect,
		ECS::Entity clipEntity) {
	auto rMin = (m_globalTransform[2] - m_absoluteSize) * invScreenSize;
	auto rMax = (m_globalTransform[2] + m_absoluteSize) * invScreenSize;
	m_clippedRect = Math::Rect(rMin, rMax);

	if (clipRect && m_absoluteRotation == 0.f) {
		m_clippedRect = clipRect->intersection(m_clippedRect);
		m_clipSource = clipEntity;

		return m_clippedRect.valid();
	}
	else {
		clipRect = nullptr;
		m_clipSource = ECS::INVALID_ENTITY;

		return true;
	}
}

void GuiObject::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen) {
	try_update_rect_transform(ecs, m_backgroundRect, invScreen);

	for (size_t i = 0; i < 4; ++i) {
		try_update_rect_transform(ecs, m_borderRects[i], invScreen);
	}
}

bool GuiObject::has_update_flag(UpdateFlag_T flag) const {
	return m_updateFlags & flag;
}

void GuiObject::set_update_flag(UpdateFlag_T flag) {
	m_updateFlags |= flag;
}

void GuiObject::clear_update_flag(UpdateFlag_T flag) {
	m_updateFlags &= ~flag;
}

void GuiObject::make_screen_transform(Math::Transform2D& result, const Math::Transform2D& input,
		const Math::Vector2& scale, const Math::Vector2& invScreen) {
	result = input;
	result[2] *= invScreen;
	result.apply_scale_local(scale * 2.f).apply_scale_global(invScreen);
}

void GuiObject::emit_rect(ECS::Manager& ecs, ECS::Entity& rectEntity, const RectInstance& rect,
		const Math::Vector2& position, const RectOrderInfo orderInfo,
		const Math::Rect* clipRect) {
	
	if (clipRect) {
		if (!clipRect->valid()) {
			try_remove_rect(ecs, rectEntity);
			return;
		}

		auto scale = Math::Vector2(rect.transform[0][0], rect.transform[1][1]) * 0.5f;
		auto clip = clipRect->intersection(
				Math::Rect(rect.transform[2] - scale, rect.transform[2] + scale));

		if (clip.valid()) {
			auto center = clip.centroid();
			auto clipScale = clip.size();

			auto rectCopy = rect;
			rectCopy.transform[0][0] = clipScale.x;
			rectCopy.transform[1][1] = clipScale.y;
			rectCopy.transform[2] = center;

			create_or_update_rect(ecs, rectEntity, rectCopy, position, orderInfo);
		}
		else {
			try_remove_rect(ecs, rectEntity);
		}
	}
	else {
		create_or_update_rect(ecs, rectEntity, rect, position, orderInfo);
	}
}

void GuiObject::create_or_update_rect(ECS::Manager& ecs, ECS::Entity& rectEntity,
		const RectInstance& rect, const Math::Vector2& position, const RectOrderInfo orderInfo) {
	if (rectEntity != ECS::INVALID_ENTITY) {
		auto& inst = ecs.get_component<RectInstance>(rectEntity);
		auto& info = ecs.get_component<RectInstanceInfo>(rectEntity);

	
		RectPriority newPriority(orderInfo);
		if (info.priority != newPriority) {
			ecs.get_or_add_component<ECS::Tag<"RectHierarchyUpdate"_hs>>(rectEntity);
		}
		else {
			g_uiRenderer->mark_rect_for_update(rectEntity);
		}

		inst = rect;
		
		info.position = position;
		info.priority = newPriority;
	}
	else {
		rectEntity = ecs.create_entity();

		RectPriority info(orderInfo);
		ecs.add_component<RectInstance>(rectEntity, rect);
		ecs.add_component<RectInstanceInfo>(rectEntity, position, RectPriority(orderInfo));
		ecs.add_component<ECS::Tag<"RectHierarchyUpdate"_hs>>(rectEntity);
		g_uiRenderer->mark_rect_for_update(rectEntity);
	}
}

void GuiObject::try_remove_rect_raw(ECS::Manager& ecs, ECS::Entity& rectEntity)
{
	if (rectEntity != ECS::INVALID_ENTITY) {
		g_uiRenderer->mark_rect_for_update(rectEntity);
		ecs.destroy_entity(rectEntity);
		rectEntity = ECS::INVALID_ENTITY;
	}
}

void GuiObject::try_remove_rect(ECS::Manager& ecs, ECS::Entity& rectEntity) {
	if (rectEntity != ECS::INVALID_ENTITY) {
		g_uiRenderer->mark_rect_for_update(rectEntity);
		uint32_t oldIndex = ECS::get_index(ecs.get_pool<RectInstanceInfo>().get_sparse_index(rectEntity));
		ecs.destroy_entity(rectEntity);
		auto& dense = ecs.get_pool<RectInstanceInfo>().get_dense();
		if (oldIndex < dense.size())
		{
			ECS::Entity newEntity = dense[oldIndex];
			if (newEntity != ECS::INVALID_ENTITY)
			{
				for (uint32_t x = oldIndex; x < dense.size(); ++x)
				{
					ecs.get_or_add_component<ECS::Tag<"RectHierarchyUpdate"_hs>>(dense[x]);
					g_uiRenderer->mark_rect_for_update(dense[x]);
				}
			}
		}
		rectEntity = ECS::INVALID_ENTITY;
	}
}

void GuiObject::try_update_rect_transform(ECS::Manager& ecs, ECS::Entity entity,
		const Math::Vector2& invScreen) {
	if (entity == ECS::INVALID_ENTITY) {
		return;
	}

	auto& rect = ecs.get_component<RectInstance>(entity);
	auto& rectInfo = ecs.get_component<RectInstanceInfo>(entity);

	rect.transform[2] = m_globalTransform * Math::Vector3(rectInfo.position, 1.f);
	rect.transform[2] *= invScreen;

	g_uiRenderer->mark_rect_for_update(entity);
}

void GuiObject::try_set_rect_color(ECS::Manager& ecs, ECS::Entity entity,
		const Math::Vector4& color) {
	if (entity == ECS::INVALID_ENTITY) {
		return;
	}

	auto& rect = ecs.get_component<RectInstance>(entity);
	rect.color = color;

	g_uiRenderer->mark_rect_for_update(entity);
}

void GuiObject::try_set_rect_sample_mode(ECS::Manager& ecs, ECS::Entity entity,
		ResamplerMode resampleMode) {
	if (entity == ECS::INVALID_ENTITY) {
		return;
	}

	auto& rect = ecs.get_component<RectInstance>(entity);
	rect.samplerIndex = static_cast<uint32_t>(resampleMode);

	g_uiRenderer->mark_rect_for_update(entity);
}

void GuiObject::set_background_color(ECS::Manager& ecs, const Math::Color3& color) {
	try_set_rect_color(ecs, m_backgroundRect, color.to_vector4(1.f - m_backgroundTransparency));
}

void GuiObject::recursive_recalc_position(ECS::Manager& ecs, Instance& selfInst,
		ECS::Entity self) {
	auto sguiEntity = selfInst.find_first_ancestor_of_class(ecs, InstanceClass::VIEWPORT_GUI);

	if (sguiEntity == ECS::INVALID_ENTITY) {
		return;
	}

	auto& screenGui = ecs.get_component<ViewportGui>(sguiEntity);

	auto guiParentEntity = selfInst.find_first_ancestor_which_is_a(ecs, InstanceClass::GUI_OBJECT);

	if (guiParentEntity == ECS::INVALID_ENTITY) {
		recalc_position(Math::Transform2D(1.f), screenGui.m_absoluteSize, 0.f,
				screenGui.m_absoluteSize);
	}
	else {
		auto& uiObjInst = ecs.get_component<Instance>(guiParentEntity);
		auto& uiObj = *try_get_gui_object(ecs, guiParentEntity, uiObjInst.m_classID);

		auto transform = uiObj.m_globalTransform;

		recalc_position(transform, uiObj.m_absoluteSize, uiObj.m_absoluteRotation,
				screenGui.m_absoluteSize);
	}

	const Math::Rect* clipRect = nullptr;

	if (m_clipSource != ECS::INVALID_ENTITY) {
		mark_for_redraw(ecs, selfInst);

		auto& clipInst = ecs.get_component<Instance>(m_clipSource);
		auto& clipObj = *try_get_gui_object(ecs, m_clipSource, clipInst.m_classID);

		clipRect = &clipObj.m_clippedRect;
	}
	else {
		ecs.get_or_add_component<ECS::Tag<"GuiTransformUpdate"_hs>>(self);
	}

	recalc_clip_rect(screenGui.m_invSize, clipRect, m_clipSource);

	for_each_child(ecs, selfInst, [&](auto childEntity, auto& childInst) {
		GuiObject* lastParent = this;

		update_child_position(ecs, childEntity, childInst, lastParent, screenGui.m_absoluteSize,
				screenGui.m_invSize, clipRect, m_clipSource);

		for_each_descendant(ecs, childInst, [&](auto entity, auto& descInst) {
			update_child_position(ecs, entity, descInst, lastParent, screenGui.m_absoluteSize,
					screenGui.m_invSize, clipRect, m_clipSource);
		});
	});
}

void GuiObject::draw_borders(ECS::Manager& ecs, const Math::Vector2& invScreen,
		RectOrderInfo orderInfo, const Math::Rect* clipRect) {
	RectInstance rect{};
	rect.color = m_borderColor3.to_vector4(1.f - m_backgroundTransparency);
	rect.samplerIndex = 1;

	float borderSizePixel = static_cast<float>(m_borderSizePixel);

	Math::Transform2D offset(1.f);
	Math::Vector2 position;
	Math::Vector2 size;

	auto drawSize = m_absoluteSize;

	switch (m_borderMode) {
		case BorderMode::MIDDLE:
			drawSize -= Math::Vector2(borderSizePixel);
			break;
		case BorderMode::INSET:
			drawSize -= Math::Vector2(2.f * borderSizePixel);
			break;
		default:
			break;
	}

	// Top
	position = Math::Vector2(0.f, -drawSize.y - borderSizePixel);
	size = Math::Vector2(drawSize.x + 2.f * borderSizePixel, borderSizePixel);
	offset.set_translation(position);
	make_screen_transform(rect.transform, m_globalTransform * offset, size, invScreen);
	emit_rect(ecs, m_borderRects[0], rect, position, orderInfo, clipRect);

	// Bottom
	position = Math::Vector2(0.f, drawSize.y + borderSizePixel);
	offset.set_translation(position);
	make_screen_transform(rect.transform, m_globalTransform * offset, size, invScreen);
	emit_rect(ecs, m_borderRects[1], rect, position, orderInfo, clipRect);

	// Left
	position = Math::Vector2(-drawSize.x - borderSizePixel, 0.f);
	size = Math::Vector2(borderSizePixel, drawSize.y);
	offset.set_translation(position);
	make_screen_transform(rect.transform, m_globalTransform * offset, size, invScreen);
	emit_rect(ecs, m_borderRects[2], rect, position, orderInfo, clipRect);

	// Right
	position = Math::Vector2(drawSize.x + borderSizePixel, 0.f);
	offset.set_translation(position);
	make_screen_transform(rect.transform, m_globalTransform * offset, size, invScreen);
	emit_rect(ecs, m_borderRects[3], rect, position, orderInfo, clipRect);
}

GuiObject* Game::try_get_gui_object(ECS::Manager& ecs, ECS::Entity entity, InstanceClass id) {
	switch (id) {
		case InstanceClass::RECT:
			return static_cast<GuiObject*>(&ecs.get_component<Rect2D>(entity));
		case InstanceClass::TEXT_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<TextRect>(entity));
		case InstanceClass::TEXT_BUTTON:
			return static_cast<GuiObject*>(&ecs.get_component<TextButton>(entity));
		case InstanceClass::INPUT_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<InputRect>(entity));
		case InstanceClass::IMAGE_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<ImageRect>(entity));
		case InstanceClass::IMAGE_BUTTON:
			return static_cast<GuiObject*>(&ecs.get_component<ImageButton>(entity));
		case InstanceClass::SCROLLING_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<ScrollingRect>(entity));
		case InstanceClass::RESIZABLE_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<ResizableRect>(entity));
		case InstanceClass::VIDEO_RECT:
			return static_cast<GuiObject*>(&ecs.get_component<VideoRect>(entity));
		default:
			return nullptr;
	}
}

