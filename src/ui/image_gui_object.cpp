#include "image_gui_object.hpp"

#include <asset/texture_cache.hpp>

#include <ecs/ecs.hpp>

using namespace Game;

void ImageGuiObject::set_image(ECS::Manager& ecs, Instance& inst, const std::string& image) {
	if (image.compare(m_image) != 0) {
		m_image = image;
		mark_for_redraw(ecs, inst);
	}
}

void ImageGuiObject::set_image_color3(ECS::Manager& ecs, const Math::Color3& color) {
	m_imageColor3 = color;

	if (is_visible()) {
		if (m_scaleType != ScaleType::SLICE) {
			try_set_rect_color(ecs, m_imageQuads[0], color.to_vector4(1.f - m_imageTransparency));
		}
		else {
			auto color4 = color.to_vector4(1.f - m_imageTransparency);

			for (size_t i = 0; i < 9; ++i) {
				try_set_rect_color(ecs, m_imageQuads[i], color4);
			}
		}
	}
}

void ImageGuiObject::set_image_rect_offset(ECS::Manager& ecs, Instance& inst,
		const Math::Vector2& imageRectOffset) {
	m_imageRectOffset = imageRectOffset;
	mark_for_redraw(ecs, inst);
}

void ImageGuiObject::set_image_rect_size(ECS::Manager& ecs, Instance& inst,
		const Math::Vector2& imageRectSize) {
	m_imageRectSize = imageRectSize;
	mark_for_redraw(ecs, inst);
}

void ImageGuiObject::set_image_transparency(ECS::Manager& ecs, Instance& inst,
		float transparency) {
	if (transparency == m_imageTransparency) {
		return;
	}

	if (transparency == 1.f || m_imageTransparency == 1.f) {
		m_imageTransparency = transparency;
		mark_for_redraw(ecs, inst);
	}
	else {
		m_imageTransparency = transparency;

		auto color4 = m_imageColor3.to_vector4(1.f - transparency);

		if (m_scaleType != ScaleType::SLICE) {
			try_set_rect_color(ecs, m_imageQuads[0], color4);
		}
		else {
			for (size_t i = 0; i < 9; ++i) {
				try_set_rect_color(ecs, m_imageQuads[i], color4);
			}
		}
	}
}

void ImageGuiObject::set_resample_mode(ECS::Manager& ecs, ResamplerMode resampleMode) {
	if (m_resampleMode != resampleMode) {
		m_resampleMode = resampleMode;

		if (is_visible()) {
			if (m_scaleType != ScaleType::SLICE) {
				try_set_rect_sample_mode(ecs, m_imageQuads[0], resampleMode);
			}
			else {
				for (size_t i = 0; i < 9; ++i) {
					try_set_rect_sample_mode(ecs, m_imageQuads[i], resampleMode);
				}
			}
		}
	}
}

void ImageGuiObject::set_scale_type(ECS::Manager& ecs, Instance& inst, ScaleType scaleType) {
	if (scaleType != m_scaleType) {
		m_scaleType = scaleType;
		mark_for_redraw(ecs, inst);
	}
}

void ImageGuiObject::set_slice_center(ECS::Manager& ecs, Instance& inst,
		const Math::Rect& sliceCenter) {
	m_sliceCenter = sliceCenter;

	if (m_scaleType == ScaleType::SLICE) {
		mark_for_redraw(ecs, inst);
	}
}

void ImageGuiObject::set_slice_scale(ECS::Manager& ecs, Instance& inst, float sliceScale) {
	if (sliceScale != m_sliceScale) {
		m_sliceScale = sliceScale;

		if (m_scaleType == ScaleType::SLICE) {
			mark_for_redraw(ecs, inst);
		}
	}
}

void ImageGuiObject::set_tile_size(ECS::Manager& ecs, Instance& inst,
		const Math::UDim2& tileSize) {
	m_tileSize = tileSize;

	if (m_scaleType == ScaleType::TILE) {
		mark_for_redraw(ecs, inst);
	}
}

void ImageGuiObject::clear(ECS::Manager& ecs) {
	GuiObject::clear(ecs);
	clear_internal(ecs);
}

void ImageGuiObject::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo,
		const ViewportGui& screenGui, const Math::Rect* clipRect) {
	GuiObject::redraw(ecs, orderInfo, screenGui, clipRect);
	redraw_internal(ecs, orderInfo, screenGui, clipRect);
}

void ImageGuiObject::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen) {
	static_cast<GuiObject*>(this)->update_rect_transforms(ecs, invScreen);

	if (m_scaleType != ScaleType::SLICE) {
		try_update_rect_transform(ecs, m_imageQuads[0], invScreen);
	}
	else {
		for (size_t i = 0; i < 9; ++i) {
			try_update_rect_transform(ecs, m_imageQuads[i], invScreen);
		}
	}
}

void ImageGuiObject::clear_internal(ECS::Manager& ecs) {
	for (size_t i = 0; i < 9; ++i) {
		try_remove_rect(ecs, m_imageQuads[i]);
	}
}

void ImageGuiObject::redraw_internal(ECS::Manager& ecs, RectOrderInfo& orderInfo,
		const ViewportGui& screenGui, const Math::Rect* clipRect) {
	if (m_imageTransparency == 1.f) {
		clear_internal(ecs);
		return;
	}

	orderInfo.zIndex = get_z_index();
	orderInfo.overlay = true;

	auto texture = g_textureCache->get_or_load<TextureLoader>(m_image, *g_renderContext, m_image,
			false, true);

	if (texture) {
		if (m_scaleType != ScaleType::SLICE) {
			draw_normal_image(ecs, m_imageQuads[0], screenGui.get_inv_size(), orderInfo,
					*texture, clipRect, m_imageColor3, m_imageTransparency, m_resampleMode,
					m_scaleType, m_tileSize, m_imageRectSize, m_imageRectOffset);
		}
		else {
			draw_9slice_image(ecs, m_imageQuads, screenGui.get_inv_size(), orderInfo,
					*texture, clipRect, m_imageColor3, m_imageTransparency, m_resampleMode,
					m_sliceCenter, m_sliceScale);
		}
	}
	else {
		clear_internal(ecs);
	}
}

