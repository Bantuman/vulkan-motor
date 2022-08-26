#pragma once

#include <core/common.hpp>
#include <core/local.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/instance_factory.hpp>
#include <core/data_model.hpp>
#include <ui/ui_components.hpp>

#include <functional>
#include <optional>

struct TreeviewEntry
{
	bool m_isCollapsed;
	uint32_t m_numChildren;
	ECS::Entity m_entity;
	ECS::Entity m_collapseButton;
	ECS::Entity m_label;
};

namespace Game::EditorFrontend {
	void init();
	void deinit();
	void update();
	inline TreeviewEntry** g_editorSelection;
	inline size_t g_editorNumSelected;
}

using namespace Game;

struct FrameCreationInfo
{
	const Math::UDim2 Size;
	const Math::UDim2 Position;
	const Math::Color3 BackgroundColor;
	const Math::Color3 OutlineColor;
	const std::optional<uint32_t> ZIndex;
	float Transparency;
};
inline ECS::Entity CreateRect(const FrameCreationInfo& aFrameInfo, const ECS::Entity aParent = ECS::INVALID_ENTITY)
{
	ECS::Entity entity;
	auto& instance = *InstanceFactory::create(*g_ecs, InstanceClass::RECT, entity);

	Rect2D& frame = g_ecs->get_component<Rect2D>(entity);
	frame.set_position(*g_ecs, instance, entity, aFrameInfo.Position);
	frame.set_size(*g_ecs, instance, aFrameInfo.Size);
	frame.set_background_transparency(*g_ecs, instance, aFrameInfo.Transparency);
	frame.set_background_color3(*g_ecs, aFrameInfo.BackgroundColor);
	frame.set_clips_descendants(*g_ecs, instance, true);
	frame.set_z_index(*g_ecs, instance, aFrameInfo.ZIndex.value_or(1));
	frame.set_border_color3(*g_ecs, aFrameInfo.OutlineColor);
	if (aParent != ECS::INVALID_ENTITY)
	{
		instance.set_parent(*g_ecs, aParent, entity);
	}
	return entity;
}

struct ResizableRectCreationInfo
{
	const Math::UDim2 Size;
	const Math::UDim2 Position;
	const Math::Color3 BackgroundColor;
	const Math::Color3 OutlineColor;
	const int16_t MaxSize;
	const int16_t MinSize;
	const ResizeDirection Direction;
	const std::optional<float> HandleWidth;
	const std::optional<uint32_t> ZIndex;
	float Transparency;
};
inline ECS::Entity CreateResizableRect(const ResizableRectCreationInfo& aFrameInfo, const ECS::Entity aParent = ECS::INVALID_ENTITY)
{
	ECS::Entity entity;
	auto& instance = *InstanceFactory::create(*g_ecs, InstanceClass::RESIZABLE_RECT, entity);

	ResizableRect& frame = g_ecs->get_component<ResizableRect>(entity);
	frame.set_position(*g_ecs, instance, entity, aFrameInfo.Position);
	frame.set_size(*g_ecs, instance, aFrameInfo.Size);
	frame.set_background_transparency(*g_ecs, instance, aFrameInfo.Transparency);
	frame.set_background_color3(*g_ecs, aFrameInfo.BackgroundColor);
	frame.set_clips_descendants(*g_ecs, instance, true);
	frame.set_z_index(*g_ecs, instance, aFrameInfo.ZIndex.value_or(1));
	frame.set_border_color3(*g_ecs, aFrameInfo.OutlineColor);
	frame.set_max_size(aFrameInfo.MaxSize);
	frame.set_min_size(aFrameInfo.MinSize);
	frame.set_direction(aFrameInfo.Direction);
	frame.set_handle_width(aFrameInfo.HandleWidth.value_or(12.f));
	if (aParent != ECS::INVALID_ENTITY)
	{
		instance.set_parent(*g_ecs, aParent, entity);
	}
	return entity;
}

struct ScrollingRectCreationInfo
{
	const Math::UDim2 Size;
	const Math::UDim2 Position;
	const Math::Color3 BackgroundColor;
	const Math::Color3 OutlineColor;
	const int16_t MaxSize;
	const int16_t MinSize;
	const ResizeDirection Direction;
	const std::optional<float> HandleWidth;
	const std::optional<uint32_t> ZIndex;
	float Transparency;
};
inline ECS::Entity CreateScrollingRect(const ScrollingRectCreationInfo& aFrameInfo, const ECS::Entity aParent = ECS::INVALID_ENTITY)
{
	ECS::Entity entity;
	auto& instance = *InstanceFactory::create(*g_ecs, InstanceClass::SCROLLING_RECT, entity);

	ScrollingRect& frame = g_ecs->get_component<ScrollingRect>(entity);
	frame.set_position(*g_ecs, instance, entity, aFrameInfo.Position);
	frame.set_size(*g_ecs, instance, aFrameInfo.Size);
	frame.set_background_transparency(*g_ecs, instance, aFrameInfo.Transparency);
	frame.set_background_color3(*g_ecs, aFrameInfo.BackgroundColor);
	frame.set_clips_descendants(*g_ecs, instance, true);
	frame.set_z_index(*g_ecs, instance, aFrameInfo.ZIndex.value_or(1));
	frame.set_border_color3(*g_ecs, aFrameInfo.OutlineColor);
	frame.set_direction(aFrameInfo.Direction);
	frame.set_handle_width(aFrameInfo.HandleWidth.value_or(12.f));
	if (aParent != ECS::INVALID_ENTITY)
	{
		instance.set_parent(*g_ecs, aParent, entity);
	}
	return entity;
}

struct TextLabelCreationInfo
{
	const char* Text;
	std::optional<Game::FontID> Font;
	std::optional<unsigned int> FontSize;
	std::optional<Game::TextXAlignment> TextXAlignment;
	std::optional<Game::TextYAlignment> TextYAlignment;
	std::optional<int32_t> ZIndex;
	std::optional<int32_t> BorderSize;
	const Math::UDim2 Size;
	const Math::UDim2 Position;
	const Math::Color3 BackgroundColor;
	const Math::Color3 OutlineColor;
	const Math::Color3 TextColor;
	std::optional<float> OutlineTransparency;
	float BackgroundTransparency;
	float TextTransparency;
};
inline ECS::Entity CreateTextLabel(const TextLabelCreationInfo& aTextInfo, const ECS::Entity aParent = ECS::INVALID_ENTITY)
{
	ECS::Entity entity;
	auto& instance = *InstanceFactory::create(*g_ecs, InstanceClass::TEXT_RECT, entity);

	TextRect& frame = g_ecs->get_component<TextRect>(entity);
	frame.set_position(*g_ecs, instance, entity, aTextInfo.Position);
	frame.set_size(*g_ecs, instance, aTextInfo.Size);
	frame.set_text_size(*g_ecs, instance, static_cast<uint8_t>(aTextInfo.FontSize.value_or(24u)));
	frame.set_line_height(*g_ecs, instance, 1.f);
	frame.set_max_visible_graphemes(*g_ecs, instance, -1);
	frame.set_z_index(*g_ecs, instance, aTextInfo.ZIndex.value_or(1));
	frame.set_border_size_pixel(*g_ecs, instance, aTextInfo.BorderSize.value_or(1));
	frame.set_background_transparency(*g_ecs, instance, aTextInfo.BackgroundTransparency);
	frame.set_text_transparency(*g_ecs, instance, aTextInfo.TextTransparency);
	frame.set_text_stroke_transparency(*g_ecs, instance, aTextInfo.OutlineTransparency.value_or(1.f));
	frame.set_font(*g_ecs, instance, aTextInfo.Font.value_or(Game::FontID::ARIAL));
	frame.set_text_x_alignment(*g_ecs, instance, aTextInfo.TextXAlignment.value_or(Game::TextXAlignment::CENTER));
	frame.set_text_y_alignment(*g_ecs, instance, aTextInfo.TextYAlignment.value_or(Game::TextYAlignment::CENTER));
	frame.set_background_color3(*g_ecs, aTextInfo.BackgroundColor);
	frame.set_border_color3(*g_ecs, aTextInfo.OutlineColor);
	frame.set_rich_text(*g_ecs, instance, false);
	frame.set_text_scaled(*g_ecs, instance, false);
	frame.set_text_color3(*g_ecs, aTextInfo.TextColor);
	frame.set_text(*g_ecs, instance, aTextInfo.Text);
	if (aParent != ECS::INVALID_ENTITY)
	{
		instance.set_parent(*g_ecs, aParent, entity);
	}
	return entity;
}

struct ImageButtonCreationInfo
{
	const char* Image;
	const char* HoverImage;
	const char* PressedImage;
	const Math::UDim2 Size;
	const Math::UDim2 Position;
	const Math::Color3 BackgroundColor;
	const Math::Color3 OutlineColor;
	const Math::Color3 ImageColor;
	float BackgroundTransparency;
	float ImageTransparency;
	std::optional<std::function<void(ECS::Entity)>> Callback;
};
inline ECS::Entity CreateImageButton(const ImageButtonCreationInfo& aButtonInfo, const ECS::Entity aParent = ECS::INVALID_ENTITY)
{
	ECS::Entity entity;
	auto& instance = *InstanceFactory::create(*g_ecs, InstanceClass::IMAGE_BUTTON, entity);

	ImageButton& frame = g_ecs->get_component<ImageButton>(entity);
	frame.set_position(*g_ecs, instance, entity, aButtonInfo.Position);
	frame.set_active(true);
	frame.set_size(*g_ecs, instance, aButtonInfo.Size);
	frame.set_background_transparency(*g_ecs, instance, aButtonInfo.BackgroundTransparency);
	frame.set_background_color3(*g_ecs, aButtonInfo.BackgroundColor);
	frame.set_image(*g_ecs, instance, aButtonInfo.Image);
	frame.set_hover_image(*g_ecs, instance, aButtonInfo.HoverImage);
	frame.set_auto_button_color(true);
	frame.set_pressed_image(*g_ecs, instance, aButtonInfo.PressedImage);
	frame.set_image_color3(*g_ecs, aButtonInfo.ImageColor);
	frame.set_image_transparency(*g_ecs, instance, aButtonInfo.ImageTransparency);
	frame.set_border_color3(*g_ecs, aButtonInfo.OutlineColor);
	frame.set_callback(std::move(aButtonInfo.Callback.value_or(nullptr)));

	if (aParent != ECS::INVALID_ENTITY)
	{
		instance.set_parent(*g_ecs, aParent, entity);
	}
	return entity;
}