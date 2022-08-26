#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/instance.hpp>
#include <ui/viewport_gui.hpp>
#include <ui/ui_enumerations.hpp>
#include <ui/ui_render_components.hpp>

#include <math/vector2.hpp>
#include <math/vector4.hpp>
#include <math/transform2d.hpp>
#include <math/color3.hpp>
#include <math/udim.hpp>
#include <math/rect.hpp>


namespace Game {
	constexpr Math::Color3 BACKGROUND_COLOR = Math::Color3::from_rgb(16, 18, 32);
	constexpr Math::Color3 DISABLED_COLOR = Math::Color3::from_rgb(39, 42, 63);
	constexpr Math::Color3 DARK_COLOR = Math::Color3::from_rgb(19, 22, 43);
	constexpr Math::Color3 SELECTED_COLOR = Math::Color3::from_rgb(86, 88, 102);
	constexpr Math::Color3 HOVER_COLOR = Math::Color3::from_rgb(66, 68, 82);
	constexpr Math::Color3 LIGHT_COLOR = { 0.6f, 0.6f, 0.9f };

class GuiObject {
	public:
		using UpdateFlag_T = uint8_t;

		static constexpr UpdateFlag_T FLAG_NEEDS_REDRAW			= 0x1;
		static constexpr UpdateFlag_T FLAG_CHILD_NEEDS_REDRAW	= 0x2;

		void set_position(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity,
				const Math::UDim2&);
		void set_size(ECS::Manager&, Instance& selfInstance, const Math::UDim2&);
		void set_rotation(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity, float);
		void set_anchor_point(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity,
				const Math::Vector2&);

		void set_background_color3(ECS::Manager&, const Math::Color3&);
		void set_border_color3(ECS::Manager&, const Math::Color3&);
		void set_background_transparency(ECS::Manager&, Instance&, float);

		void set_border_size_pixel(ECS::Manager&, Instance& selfInstance, int32_t);
		void set_z_index(ECS::Manager&, Instance& selfInstance, int32_t);
		void set_layout_order(ECS::Manager&, Instance& selfInstance, int32_t);
		void set_size_constraint(ECS::Manager&, Instance& selfInstance, SizeConstraint);
		void set_border_mode(ECS::Manager&, Instance& selfInstance, BorderMode);
		void set_automatic_size(ECS::Manager&, Instance& selfInstance, AutomaticSize);

		void set_visible(ECS::Manager&, Instance& selfInstance, bool);
		void set_clips_descendants(ECS::Manager&, Instance& selfInstance, bool);
		void set_active(bool);

		void set_mouse_inside(bool);

		const Math::UDim2& get_size() const;
		const Math::UDim2& get_position() const;
		const Math::Vector2& get_absolute_position() const;
		const Math::Vector2& get_absolute_size() const;
		float get_absolute_rotation() const;
		const Math::Transform2D& get_global_transform() const;
		const Math::Rect& get_clipped_rect() const;
		ECS::Entity get_clip_source() const;

		uint32_t get_z_index() const;

		const Math::Color3& get_background_color3() const;

		const RectPriority& get_priority() const;

		bool is_visible() const;
		bool clips_descendants() const;
		bool is_active() const;
		bool is_mouse_inside() const;

		virtual bool contains_point(const Math::Vector2& point, const Math::Vector2& invScreen) const;

		void mark_for_redraw(ECS::Manager&, Instance&);

		virtual void clear(ECS::Manager&);
		virtual void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
				const Math::Rect* clipRect);

		void recursive_clear(ECS::Manager&, Instance&);

		void recalc_size(const Math::Vector2& parentScale);
		void recalc_position(const Math::Transform2D& parentTransform,
				const Math::Vector2& parentScale, float parentRotation,
				const Math::Vector2& screenSize);
		bool recalc_clip_rect(const Math::Vector2& invScreenSize, const Math::Rect* clipRect,
				ECS::Entity clipEntity);

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);

		bool has_update_flag(UpdateFlag_T) const;

		void recursive_recalc_position(ECS::Manager&, Instance& selfInst, ECS::Entity selfEntity);
		void set_update_flag(UpdateFlag_T);
		void clear_update_flag(UpdateFlag_T);
	protected:
		static void make_screen_transform(Math::Transform2D& result, const Math::Transform2D& input,
				const Math::Vector2& scale, const Math::Vector2& invScreen);

		static void emit_rect(ECS::Manager&, ECS::Entity&, const RectInstance&,
				const Math::Vector2& position, const RectOrderInfo, const Math::Rect* clipRect);
		static void create_or_update_rect(ECS::Manager&, ECS::Entity&, const RectInstance&,
				const Math::Vector2& position, const RectOrderInfo);
		static void try_remove_rect(ECS::Manager&, ECS::Entity&);
		static void try_remove_rect_raw(ECS::Manager& ecs, ECS::Entity& rectEntity);

		void try_update_rect_transform(ECS::Manager&, ECS::Entity, const Math::Vector2& invScreen);
		void try_set_rect_color(ECS::Manager&, ECS::Entity, const Math::Vector4&);
		void try_set_rect_sample_mode(ECS::Manager&, ECS::Entity, ResamplerMode);

		void set_background_color(ECS::Manager&, const Math::Color3&);

		Math::Transform2D m_globalTransform;
		Math::Vector2 m_absolutePosition;
		Math::Vector2 m_absoluteSize;
		float m_absoluteRotation;
		uint32_t m_zIndex;
	private:
		Math::UDim2 m_position = {};
		Math::UDim2 m_size = {{0.f, 100}, {0.f, 100}};

		Math::Rect m_clippedRect;

		RectPriority m_priority;
		ECS::Entity m_clipSource = ECS::INVALID_ENTITY;
		ECS::Entity m_backgroundRect = ECS::INVALID_ENTITY;
		ECS::Entity m_borderRects[4] = {ECS::INVALID_ENTITY, ECS::INVALID_ENTITY,
				ECS::INVALID_ENTITY, ECS::INVALID_ENTITY};

		Math::Color3 m_backgroundColor3 = Math::Color3(1.f, 1.f, 1.f);
		Math::Color3 m_borderColor3 = Math::Color3::from_rgb(27.f, 42.f, 53.f);
		Math::Vector2 m_anchorPoint = {};
		float m_backgroundTransparency = 0.f;
		float m_rotation = 0.f;
		int32_t m_borderSizePixel = 1;
		int32_t m_layoutOrder = 0;
		SizeConstraint m_sizeConstraint = SizeConstraint::RELATIVE_XY;
		BorderMode m_borderMode = BorderMode::OUTLINE;
		AutomaticSize m_automaticSize = AutomaticSize::NONE;
		bool m_visible = true;
		bool m_clipsDescendants = false;
		bool m_active = false;
		bool m_mouseInside = false;

		UpdateFlag_T m_updateFlags = 0;

		
		void draw_borders(ECS::Manager&, const Math::Vector2& invScreen, RectOrderInfo,
				const Math::Rect* clipRect);
};

GuiObject* try_get_gui_object(ECS::Manager&, ECS::Entity, InstanceClass);

}

