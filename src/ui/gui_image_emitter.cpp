#include "gui_image_emitter.hpp"

#include <ecs/ecs.hpp>

#include <rendering/renderer/ui_renderer.hpp>

#include <math/common.hpp>
#include <math/geometric.hpp>

#include <rendering/texture.hpp>

using namespace Game;

void GuiImageEmitter::draw_normal_image(ECS::Manager& ecs, ECS::Entity& imgEntity,
		const Math::Vector2& invScreen, const RectOrderInfo& orderInfo,
		Texture& texture, const Math::Rect* clipRect, const Math::Color3& imageColor,
		float imageTransparency, ResamplerMode sampleMode, ScaleType scaleType,
	  	const Math::UDim2& tileSize, const Math::Vector2& imageRectSize,
		const Math::Vector2& imageRectOffset) {
	RectInstance rect{};
	rect.color = imageColor.to_vector4(1.f - imageTransparency);
	rect.samplerIndex = static_cast<uint32_t>(sampleMode);
	rect.imageIndex = g_uiRenderer->get_image_index(*texture.get_image_view());

	Math::Vector2 screenSize;

	switch (scaleType) {
		case ScaleType::STRETCH:
			screenSize = get_absolute_size();
			rect.texLayout = Math::Vector4(0, 0, 1, 1);
			break;
		case ScaleType::TILE:
		{
			Math::Vector2 resTileSize(0.f, 0.f);

			if (tileSize.x.scale != 0.f) {
				resTileSize.x += 1.f / tileSize.x.scale;
			}

			if (tileSize.y.scale != 0.f) {
				resTileSize.y += 1.f / tileSize.y.scale;
			}

			if (tileSize.x.offset != 0) {
				resTileSize.x += get_absolute_size().x / static_cast<float>(tileSize.x.offset);
			}

			if (tileSize.y.offset != 0) {
				resTileSize.y += get_absolute_size().y / static_cast<float>(tileSize.y.offset);
			}

			screenSize = get_absolute_size();
			rect.texLayout = Math::Vector4(0, 0, resTileSize);
		}
			break;
		case ScaleType::FIT:
		{
			auto extent = texture.get_image()->get_extent();
			auto texAspectRatio = static_cast<float>(extent.width)
					/ static_cast<float>(extent.height);
			auto uiAspectRatio = get_absolute_size().x / get_absolute_size().y;

			if (texAspectRatio < uiAspectRatio) {
				screenSize = Math::Vector2(get_absolute_size().y * texAspectRatio,
						get_absolute_size().y);
			}
			else if (texAspectRatio > uiAspectRatio) {
				screenSize = Math::Vector2(get_absolute_size().x,
						get_absolute_size().x / texAspectRatio);
			}
			else {
				screenSize = get_absolute_size();
			}

			rect.texLayout = Math::Vector4(0, 0, 1, 1);
		}
			break;
		case ScaleType::CROP:
		{
			auto extent = texture.get_image()->get_extent();
			auto texAspectRatio = static_cast<float>(extent.width)
					/ static_cast<float>(extent.height);
			auto uiAspectRatio = get_absolute_size().x / get_absolute_size().y;

			if (texAspectRatio < uiAspectRatio) {
				float scl = texAspectRatio / uiAspectRatio;
				rect.texLayout = Math::Vector4(0, 0.5f - 0.5f * scl, 1, scl);
			}
			else if (texAspectRatio > uiAspectRatio) {
				float scl = uiAspectRatio / texAspectRatio;
				rect.texLayout = Math::Vector4(0.5f - 0.5f * scl, 0, scl, 1);
			}
			else {
				rect.texLayout = Math::Vector4(0, 0, 1, 1);
			}

			screenSize = get_absolute_size();
		}
			break;
		default:
			break;
	}

	if (imageRectSize.x != 0.f || imageRectSize.x != 0.f
			|| imageRectOffset.x != 0.f || imageRectOffset.y != 0.f) {
		auto extent = texture.get_image()->get_extent();
		Math::Vector2 invTexSize(1.f / static_cast<float>(extent.width),
				1.f / static_cast<float>(extent.height));

		// FIXME: max(values, 0)
		Math::Vector2 texOffset(rect.texLayout.x, rect.texLayout.y);
		Math::Vector2 texScale(rect.texLayout.z, rect.texLayout.w);
		Math::Vector2 rectOffset = imageRectOffset * invTexSize;
		Math::Vector2 rectSize = glm::min(imageRectSize * invTexSize,
				Math::Vector2(1.f, 1.f) - rectOffset);

		rect.texLayout = Math::Vector4(texOffset + rectOffset * texScale,
				texScale * rectSize);
	}

	make_screen_transform(rect.transform, get_global_transform(), screenSize, invScreen);
	emit_rect(ecs, imgEntity, rect, Math::Vector2(), orderInfo, clipRect);
}

void GuiImageEmitter::draw_9slice_image(ECS::Manager& ecs, ECS::Entity* imgEntities,
		const Math::Vector2& invScreen, const RectOrderInfo& orderInfo,
		Texture& texture, const Math::Rect* clipRect, const Math::Color3& imageColor,
		float imageTransparency, ResamplerMode sampleMode, const Math::Rect& sliceCenter,
		float sliceScale) {
	RectInstance rect{};
	rect.color = imageColor.to_vector4(1.f - imageTransparency);
	rect.samplerIndex = static_cast<uint32_t>(sampleMode);
	rect.imageIndex = g_uiRenderer->get_image_index(*texture.get_image_view());

	auto extent = texture.get_image()->get_extent();
	Math::Vector2 texSize(static_cast<float>(extent.width), static_cast<float>(extent.height));

	float xSize0 = sliceScale * sliceCenter.xMin;
	float ySize0 = sliceScale * sliceCenter.yMin;

	float xSize1 = sliceScale * (texSize.x - sliceCenter.xMax);
	float ySize1 = sliceScale * (texSize.y - sliceCenter.yMax);

	float xSizeMid = get_absolute_size().x - xSize0 - xSize1;
	float ySizeMid = get_absolute_size().y - ySize0 - ySize1;

	if (xSizeMid < 0.f || ySizeMid < 0.f) {
		float scl = fminf(get_absolute_size().x / (xSize0 + xSize1),
				get_absolute_size().y / (ySize0 + ySize1));

		xSize0 *= scl;
		ySize0 *= scl;
		xSize1 *= scl;
		ySize1 *= scl;

		xSizeMid = fmaxf(get_absolute_size().x - xSize0 - xSize1, 0.f);
		ySizeMid = fmaxf(get_absolute_size().y - ySize0 - ySize1, 0.f);
	}

	Math::Transform2D offset(1.f);

	// Center
	if (xSizeMid > 0.f && ySizeMid > 0.f) {
		Math::Vector2 localSize(xSizeMid, ySizeMid);
		Math::Vector2 localPos(xSize0, ySize0);

		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		rect.texLayout = Math::Vector4(sliceCenter.xMin, sliceCenter.yMin,
				sliceCenter.xMax - sliceCenter.xMin,
				sliceCenter.yMax - sliceCenter.yMin)
				/ Math::Vector4(texSize, texSize);

		emit_rect(ecs, imgEntities[0], rect, localPos, orderInfo, clipRect);
	}

	// Top Left
	if (xSize0 > 0.f && ySize0 > 0.f) {
		auto normUV = Math::Vector2(sliceCenter.xMin, sliceCenter.yMin) / texSize;
		rect.texLayout = Math::Vector4(0.f, 0.f, normUV);

		Math::Vector2 localSize(xSize0, ySize0);
		offset.set_translation(localSize - get_absolute_size());
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);
		
		emit_rect(ecs, imgEntities[1], rect, localSize - get_absolute_size(), orderInfo, clipRect);
	}

	// Top Right
	if (xSize1 > 0.f && ySize0 > 0.f) {
		rect.texLayout = Math::Vector4(sliceCenter.xMax, 0.f,
				texSize.x - sliceCenter.xMax, sliceCenter.yMin)
				/ Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSize1, ySize0);
		Math::Vector2 localPos(xSize0 + xSizeMid, 0.f);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[2], rect, localPos, orderInfo, clipRect);
	}

	// Bottom Left
	if (xSize0 > 0.f && ySize1 > 0.f) {
		rect.texLayout = Math::Vector4(0.f, sliceCenter.yMax,
				sliceCenter.xMin, texSize.y - sliceCenter.yMax)
				/ Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSize0, ySize1);
		Math::Vector2 localPos(0.f, ySize0 + ySizeMid);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[3], rect, localPos, orderInfo, clipRect);
	}

	// Bottom Right
	if (xSize1 > 0.f && ySize1 > 0.f) {
		auto normUV = Math::Vector2(sliceCenter.xMax, sliceCenter.yMax) / texSize;
		rect.texLayout = Math::Vector4(normUV, Math::Vector2(1.f) - normUV);

		Math::Vector2 localSize(xSize1, ySize1);
		Math::Vector2 localPos(xSize0 + xSizeMid, ySize0 + ySizeMid);
		offset.set_translation(2.f * localPos + localSize - get_absolute_size());
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[4], rect, localPos, orderInfo, clipRect);
	}

	// Top Mid
	if (ySize0 > 0.f) {
		rect.texLayout = Math::Vector4(sliceCenter.xMin, 0.f,
				sliceCenter.xMax - sliceCenter.xMin, sliceCenter.yMin)
				/ Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSizeMid, ySize0);
		Math::Vector2 localPos(xSize0, 0.f);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);
		
		emit_rect(ecs, imgEntities[5], rect, localPos, orderInfo, clipRect);
	}

	// Bottom Mid
	if (ySize1 > 0.f) {
		rect.texLayout = Math::Vector4(sliceCenter.xMin, sliceCenter.yMax,
				sliceCenter.xMax - sliceCenter.xMin,
				texSize.y - sliceCenter.yMax) / Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSizeMid, ySize1);
		Math::Vector2 localPos(xSize0, ySize0 + ySizeMid);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[6], rect, localPos, orderInfo, clipRect);
	}

	// Left Mid
	if (xSize0 > 0.f) {
		rect.texLayout = Math::Vector4(0.f, sliceCenter.yMin, sliceCenter.xMin,
				sliceCenter.yMax - sliceCenter.yMin)
				/ Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSize0, ySizeMid);
		Math::Vector2 localPos(0.f, ySize0);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[7], rect, localPos, orderInfo, clipRect);
	}

	// Right Mid
	if (xSize1 > 0.f) {
		rect.texLayout = Math::Vector4(sliceCenter.xMax, sliceCenter.yMin,
				texSize.x - sliceCenter.xMax, sliceCenter.yMax - sliceCenter.yMin)
				/ Math::Vector4(texSize, texSize);

		Math::Vector2 localSize(xSize1, ySizeMid);
		Math::Vector2 localPos(xSize0 + xSizeMid, ySize0);
		localPos = 2.f * localPos + localSize - get_absolute_size();
		offset.set_translation(localPos);
		make_screen_transform(rect.transform, get_global_transform() * offset, localSize,
				invScreen);

		emit_rect(ecs, imgEntities[8], rect, localPos, orderInfo, clipRect);
	}
}

