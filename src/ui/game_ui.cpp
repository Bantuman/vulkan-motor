#include "game_ui.hpp"

#include <cassert>

#include <core/application.hpp>
#include <core/logging.hpp>
#include <core/hashed_string.hpp>

#include <ecs/ecs.hpp>

#include <core/data_model.hpp>
#include <core/instance_utils.hpp>
#include <core/context_action.hpp>
#include <ui/ui_components.hpp>
#include <ui/text_gui_object.hpp>

#include <rendering/render_context.hpp>

using namespace Game;

static void draw_children(ECS::Manager& ecs, const Math::Transform2D& parentTransform,
		const Math::Vector2& parentScale, float parentRotation, ViewportGui& screenGui,
		ECS::Entity entity, Instance& inst, RectOrderInfo orderInfo, const Math::Rect* clipRect,
		ECS::Entity clipEntity);

static void update_ui_input(ECS::Manager& ecs, const Math::Vector2& screenSize, const Math::Vector2& invScreen, float deltaTime);

static void on_mouse_button_1_down(ECS::Manager& ecs, const InputObject& input);
static void on_mouse_button_1_up(ECS::Manager& ecs, const InputObject& input);

static void on_swap_chain_resized(ECS::Manager& ecs, int width, int height);

template <typename GuiClass>
static void resolve_transform_update(ECS::Manager& ecs) {
	ecs.run_system<GuiClass, Instance, ECS::Tag<"GuiTransformUpdate"_hs>>([&](auto, auto& gui,
			auto& inst, auto&) {
		auto sguiEntity = inst.find_first_ancestor_of_class(ecs, InstanceClass::VIEWPORT_GUI);

		if (sguiEntity != ECS::INVALID_ENTITY) {
			auto& screenGui = ecs.get_component<ViewportGui>(sguiEntity);
			gui.update_rect_transforms(ecs, screenGui.get_inv_size());
		}
	});
}

void Game::init_ui(ECS::Manager& ecs) {
	g_contextActionManager->bind_action_at_priority("CoreGui_MouseButton1",
			UserInputType::MOUSE_BUTTON_1, 0, ContextActionPriority::LOW, [&](const auto& input) {
		switch (input.state) {
			case UserInputState::BEGIN:
				on_mouse_button_1_down(ecs, input);
				break;
			case UserInputState::END:
				on_mouse_button_1_up(ecs, input);
				break;
			default:
				break;
		}

		return ContextActionResult::SINK;
	});

	g_renderContext->swapchain_resize_event().connect([&](int width, int height) {
		on_swap_chain_resized(ecs, width, height);
	});
}

static void draw_screen_gui_collection(ECS::Manager& ecs, Instance& collection,
		RectOrderInfo orderInfo, const Math::Vector2& screenSize) {
	for_each_child(ecs, collection, [&](auto entity, auto& inst) {
		if (inst.m_classID == InstanceClass::VIEWPORT_GUI) {
			auto& sGui = ecs.get_component<ViewportGui>(entity);

			orderInfo.displayOrder = sGui.get_display_order();
			orderInfo.zIndexBehavior = sGui.get_z_index_behavior();
			orderInfo.depth = 0;
			orderInfo.childIndex = 0;

			if (sGui.needs_redraw()) {
				sGui.clear_redraw();
				sGui.set_absolute_position(Math::Vector2());
				sGui.set_absolute_size(screenSize);
				draw_children(ecs, Math::Transform2D(1.f), screenSize, 0.f, sGui, entity, inst,
						orderInfo, nullptr, ECS::INVALID_ENTITY);
			}

			++orderInfo.screenIndex;
		}
	});
}

void Game::update_ui(ECS::Manager& ecs, float deltaTime) {
	resolve_transform_update<Rect2D>(ecs);
	resolve_transform_update<TextRect>(ecs);
	resolve_transform_update<InputRect>(ecs);
	resolve_transform_update<TextButton>(ecs);
	resolve_transform_update<ImageRect>(ecs);
	resolve_transform_update<ImageButton>(ecs);
	resolve_transform_update<ScrollingRect>(ecs);
	resolve_transform_update<ResizableRect>(ecs);

	ecs.clear_pool<ECS::Tag<"GuiTransformUpdate"_hs>>();

	auto eCoreGui = ECS::INVALID_ENTITY;
	auto* instCoreGui = g_game->get_singleton(InstanceClass::ENGINE_GUI, eCoreGui);

	auto eStarterGui = ECS::INVALID_ENTITY;
	auto* instStarterGui = g_game->get_singleton(InstanceClass::GAME_GUI, eStarterGui);

	auto extents = g_renderContext->get_swapchain_extent();
	float width = static_cast<float>(extents.width);
	float height = static_cast<float>(extents.height);

	RectOrderInfo orderInfo;
	orderInfo.screenIndex = 0;

	Math::Vector2 screenSize(width, height);
	auto invScreenSize = 1.f / screenSize;

	draw_screen_gui_collection(ecs, *instCoreGui, orderInfo, screenSize);
	orderInfo.screenIndex = 0;
	draw_screen_gui_collection(ecs, *instStarterGui, orderInfo, screenSize);

	update_ui_input(ecs, screenSize, invScreenSize, deltaTime);
}

static void draw_children(ECS::Manager& ecs, const Math::Transform2D& parentTransform,
		const Math::Vector2& parentScale, float parentRotation, ViewportGui& screenGui,
		ECS::Entity entity, Instance& inst, RectOrderInfo orderInfo, const Math::Rect* clipRect,
		ECS::Entity clipEntity) {
	if (inst.is_a(InstanceClass::GUI_OBJECT)) {

		GuiObject* uiObj = try_get_gui_object(ecs, entity, inst.m_classID);
		assert(uiObj && "GuiObject component must be present for this instance");

		if (uiObj->has_update_flag(GuiObject::FLAG_NEEDS_REDRAW)) {
			if (!uiObj->is_visible() || !screenGui.is_enabled()) {
				uiObj->recursive_clear(ecs, inst);
				return;
			}

			uiObj->recalc_size(parentScale);
			uiObj->recalc_position(parentTransform, parentScale, parentRotation,
					screenGui.get_absolute_size());

			if (!uiObj->recalc_clip_rect(screenGui.get_inv_size(), clipRect, clipEntity)) {
				uiObj->recursive_clear(ecs, inst);
				return;
			}

			uiObj->redraw(ecs, orderInfo, screenGui, clipRect);
		}

		if (uiObj->has_update_flag(GuiObject::FLAG_NEEDS_REDRAW
				| GuiObject::FLAG_CHILD_NEEDS_REDRAW)) {
			if (uiObj->clips_descendants() && uiObj->get_absolute_rotation() == 0.f) {
				clipEntity = entity;
				clipRect = &uiObj->get_clipped_rect();
			}

			orderInfo.childIndex = 0;
			++orderInfo.depth;

			for_each_child(ecs, inst, [&](auto entity, auto& child) {
				auto transform = uiObj->get_global_transform();
				if (ScrollingRect* rect = dynamic_cast<ScrollingRect*>(uiObj))
				{
					/*Math::Transform2D scrollingOffset(1.f);
					scrollingOffset.set_translation({ 0, -rect->get_offset() });
					transform *= scrollingOffset;*/
					transform[2] += Math::Vector2{0, -rect->get_offset()};
				}
				draw_children(ecs, transform, uiObj->get_absolute_size(),
						uiObj->get_absolute_rotation(), screenGui, entity, child, orderInfo,
						clipRect, clipEntity);
				++orderInfo.childIndex;
			});
		}

		uiObj->clear_update_flag(GuiObject::FLAG_NEEDS_REDRAW
				| GuiObject::FLAG_CHILD_NEEDS_REDRAW);
	}
	else {
		orderInfo.childIndex = 0;
		++orderInfo.depth;

		for_each_child(ecs, inst, [&](auto entity, auto& child) {
			draw_children(ecs, parentTransform, parentScale, parentRotation, screenGui,
					entity, child, orderInfo, clipRect, clipEntity);
			++orderInfo.childIndex;
		});
	}
}

template <typename GuiClass>
static void update_mouse_occupancy(ECS::Manager& ecs, const Math::Vector2& mousePos, const Math::Vector2& invScreen) {
	ecs.run_system<GuiClass>([&](auto entity, auto& uiObj) {
		bool mouseInside = uiObj.contains_point(mousePos, invScreen);
		bool lastMouseInside = uiObj.is_mouse_inside();

		uiObj.set_mouse_inside(mouseInside);

		if (mouseInside) {
			ecs.add_component<ECS::Tag<"MouseInside"_hs>>(entity);
		}

		if (!lastMouseInside && mouseInside) {
			ecs.add_component<ECS::Tag<"MouseEnter"_hs>>(entity);
		}
		else if (lastMouseInside && !mouseInside) {
			ecs.add_component<ECS::Tag<"MouseLeave"_hs>>(entity);
		}
	});
}

static void update_auto_button_color(ECS::Manager& ecs) 
{
	ecs.run_system<TextButton, Instance, ECS::Tag<"MouseEnter"_hs>>([&](auto, auto& uiObj, auto& inst, auto&) 
	{
		uiObj.set_button_hovered(ecs, inst);
	});

	ecs.run_system<ImageButton, Instance, ECS::Tag<"MouseEnter"_hs>>([&](auto, auto& uiObj, auto& inst, auto&) 
	{
		uiObj.set_button_hovered(ecs, inst);
	});

	ecs.run_system<TextButton, Instance, ECS::Tag<"MouseLeave"_hs>>([&](auto, auto& uiObj, auto& inst, auto&) 
	{
		uiObj.set_button_normal(ecs, inst);
	});

	ecs.run_system<ImageButton, Instance, ECS::Tag<"MouseLeave"_hs>>([&](auto, auto& uiObj, auto& inst, auto&)
	{
		uiObj.set_button_normal(ecs, inst);
	});
}

static void update_dynamic_rects(ECS::Manager& ecs, const Math::Vector2& rawMousePos, const Math::Vector2& mousePos, float deltaTime)
{
	ecs.run_system<ResizableRect, Instance>([&](auto, auto& uiObj, auto& inst) 
	{
		uiObj.update(ecs, inst, rawMousePos, deltaTime);
	});
	ecs.run_system<ScrollingRect, Instance>([&](auto uiEnt, auto& uiObj, auto& inst)
	{
		uiObj.update(ecs, inst, uiEnt, mousePos, deltaTime);
	});
}

static void update_ui_input(ECS::Manager& ecs, const Math::Vector2& screenSize, const Math::Vector2& invScreen, float deltaTime) {
	auto mousePos = 2.f * g_application->get_mouse_position() - screenSize;

	ecs.clear_pool<ECS::Tag<"MouseEnter"_hs>>();
	ecs.clear_pool<ECS::Tag<"MouseLeave"_hs>>();
	ecs.clear_pool<ECS::Tag<"MouseInside"_hs>>();

	update_mouse_occupancy<Rect2D>(ecs, mousePos, invScreen);
	update_mouse_occupancy<TextRect>(ecs, mousePos, invScreen);
	update_mouse_occupancy<InputRect>(ecs, mousePos, invScreen);
	update_mouse_occupancy<TextButton>(ecs, mousePos, invScreen);
	update_mouse_occupancy<ImageRect>(ecs, mousePos, invScreen);
	update_mouse_occupancy<ImageButton>(ecs, mousePos, invScreen);
	update_mouse_occupancy<ScrollingRect>(ecs, mousePos, invScreen);
	update_mouse_occupancy<ResizableRect>(ecs, mousePos, invScreen);

	update_auto_button_color(ecs);
	update_dynamic_rects(ecs, g_application->get_mouse_position(), mousePos, deltaTime);
}

static void on_mouse_button_1_down(ECS::Manager& ecs, const InputObject& input) {
	(void)input;

	RectPriority priority{};
	GuiButton* button = nullptr;
	ECS::Entity entity = ECS::INVALID_ENTITY;

	ecs.run_system<TextButton, Instance, ECS::Tag<"MouseInside"_hs>>([&](auto ent, auto& uiObj,
			auto& inst, auto&) {
		uiObj.set_button_pressed(ecs, inst);

		if (!button || priority < uiObj.get_priority()) {
			priority = uiObj.get_priority();
			button = &uiObj;
			entity = ent;
		}
	});

	ecs.run_system<ImageButton, Instance, ECS::Tag<"MouseInside"_hs>>([&](auto ent, auto& uiObj,
			auto& inst, auto&) {
		uiObj.set_button_pressed(ecs, inst);
		 
		if (!button || priority < uiObj.get_priority()) {
			priority = uiObj.get_priority();
			button = &uiObj;
			entity = ent;
		}
	});

	if (button) {
		if (auto callback = button->get_callback())
		{
			callback(entity);
		}
		ecs.add_component<ECS::Tag<"MouseDown"_hs>>(entity);
		LOG_TEMP("MOUSE DOWN ON %p", button);
	}
}

static void on_mouse_button_1_up(ECS::Manager& ecs, const InputObject& input) {
	(void)input;

	RectPriority priority{};
	GuiButton* button = nullptr;

	ecs.run_system<TextButton, Instance, ECS::Tag<"MouseInside"_hs>>([&](auto, auto& uiObj,
				auto& inst, auto&) {
		uiObj.set_button_normal(ecs, inst);

		if (!button || priority < uiObj.get_priority()) {
			priority = uiObj.get_priority();
			button = &uiObj;
		}
	});

	ecs.run_system<ImageButton, Instance, ECS::Tag<"MouseInside"_hs>>([&](auto, auto& uiObj,
				auto& inst, auto&) {
		uiObj.set_button_normal(ecs, inst);

		if (!button || priority < uiObj.get_priority()) {
			priority = uiObj.get_priority();
			button = &uiObj;
		}
	});

	ecs.clear_pool<ECS::Tag<"MouseDown"_hs>>();
	if (button) {
		LOG_TEMP("MOUSE UP ON %p", button);
	}
}

static void on_swap_chain_resized(ECS::Manager& ecs, int iWidth, int iHeight) {
	auto eStarterGui = ECS::INVALID_ENTITY;
	auto* instStarterGui = g_game->get_singleton(InstanceClass::GAME_GUI, eStarterGui);

	float width = static_cast<float>(iWidth);
	float height = static_cast<float>(iHeight);

	Math::Vector2 screenSize(width, height);

	for_each_child(ecs, *instStarterGui, [&](auto entity, auto& inst) {
		if (inst.m_classID == InstanceClass::VIEWPORT_GUI) {
			ViewportGui& sGui = ecs.get_component<ViewportGui>(entity);
			sGui.set_absolute_position(Math::Vector2());
			sGui.set_absolute_size(screenSize);
			sGui.mark_for_redraw(*g_ecs, inst);
		}
	});

	auto eCoreGui = ECS::INVALID_ENTITY;
	auto* instCoreGui = g_game->get_singleton(InstanceClass::ENGINE_GUI, eCoreGui);

	for_each_child(ecs, *instCoreGui, [&](auto entity, auto& inst) {
		if (inst.m_classID == InstanceClass::VIEWPORT_GUI) {
			ViewportGui& sGui = ecs.get_component<ViewportGui>(entity);
			sGui.set_absolute_position(Math::Vector2());
			sGui.set_absolute_size(screenSize);
			sGui.mark_for_redraw(*g_ecs, inst);
		}
	});
}

