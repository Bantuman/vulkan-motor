#include "frame.hpp"

#include <asset/texture_cache.hpp>

#include <ecs/ecs.hpp>

using namespace Game;

static const char* get_frame_image_path(FrameStyle frameStyle);

void Rect2D::set_frame_style(ECS::Manager& ecs, Instance& inst, FrameStyle frameStyle) {
	if (frameStyle != m_frameStyle) {
		m_frameStyle = frameStyle;
		mark_for_redraw(ecs, inst);
	}
}

void Rect2D::clear(ECS::Manager& ecs) {
	if (m_frameStyle == FrameStyle::CUSTOM) {
		GuiObject::clear(ecs);
	}
	else {
		clear_internal(ecs);
	}
}

void Rect2D::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& viewportGui,
		const Math::Rect* clipRect) {
	if (m_frameStyle == FrameStyle::CUSTOM) {
		GuiObject::redraw(ecs, std::move(orderInfo), viewportGui, clipRect);
	}
	else {
		redraw_internal(ecs, orderInfo, viewportGui, clipRect);
	}
}

void Rect2D::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen) {
	if (m_frameStyle == FrameStyle::CUSTOM) {
		static_cast<GuiObject*>(this)->update_rect_transforms(ecs, invScreen);
	}
	else {
		for (size_t i = 0; i < 9; ++i) {
			try_update_rect_transform(ecs, m_imageQuads[i], invScreen);
		}
	}
}

void Rect2D::clear_internal(ECS::Manager& ecs) {
	for (size_t i = 0; i < 9; ++i) {
		try_remove_rect(ecs, m_imageQuads[i]);
	}
}

void Rect2D::redraw_internal(ECS::Manager& ecs, RectOrderInfo& orderInfo,
		const ViewportGui& screenGui, const Math::Rect* clipRect) {
	const char* imagePath = get_frame_image_path(m_frameStyle);

	if (!imagePath) {
		clear_internal(ecs);
		return;
	}

	auto texture = g_textureCache->get_or_load<TextureLoader>(imagePath, *g_renderContext,
			imagePath, false, true);

	if (!texture) {
		clear_internal(ecs);
		return;
	}

	Math::Rect sliceCenter(8.f, 8.f, 32.f, 32.f);
	float sliceScale = 1.f;

	if (m_frameStyle == FrameStyle::SQUARE) {
		sliceCenter = Math::Rect(8.f, 8.f, 16.f, 16.f);
		sliceScale = 0.f;
	}
	else if (m_frameStyle == FrameStyle::ROUND) {
		sliceCenter = Math::Rect(8.f, 8.f, 16.f, 16.f);
	}

	orderInfo.zIndex = get_z_index();
	orderInfo.overlay = true;

	draw_9slice_image(ecs, m_imageQuads, screenGui.get_inv_size(), orderInfo, *texture, clipRect,
			Math::Color3(1.f, 1.f, 1.f), 0.f, ResamplerMode::PIXELATED, sliceCenter, sliceScale);
}

static const char* get_frame_image_path(FrameStyle frameStyle) {
	switch (frameStyle) {
		case FrameStyle::CHAT_BLUE:
			return "rbxasset://Textures/ui/dialog_blue.png";
		case FrameStyle::SQUARE:
		case FrameStyle::ROUND:
			return "rbxasset://Textures/ui/default_ui_square.png";
		case FrameStyle::CHAT_GREEN:
			return "rbxasset://Textures/ui/dialog_green.png";
		case FrameStyle::CHAT_RED:
			return "rbxasset://Textures/ui/dialog_red.png";
		case FrameStyle::DROP_SHADOW:
			return "rbxasset://Textures/ui/newBkg_square.png";
		default:
			return nullptr;
	}
}

