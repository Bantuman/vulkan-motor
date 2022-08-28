#include "editor_ui.hpp"

#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/instance_factory.hpp>
#include <core/data_model.hpp>
#include <ui/frame.hpp>
#include <ui/text_label.hpp>
#include <core/hashed_string.hpp>

#include <rendering/vk_profiler.hpp>

#include "properties.hpp"



static void RecursiveInit(ECS::Entity eInstance, int aDepth = 0)
{
	//Properties::properties_init();
	//auto& props = Properties::g_propertyTable["Test"];
	//(void)props.size();
	Instance* aInstance = &g_ecs->get_component<Instance>(eInstance);
	TreeviewEntry treeviewEntry{ 
		.m_isCollapsed = false,
		.m_numChildren = 0,
		.m_entity = eInstance, 
		.m_collapseButton = ECS::INVALID_ENTITY,
		.m_label = ECS::INVALID_ENTITY,
	};
	aInstance = &g_ecs->get_component<Instance>(eInstance);
	if (aInstance->get_class_name().compare("CoreEditorGui") == 0)
	{
		return;
	}
	auto nextEntity = aInstance->m_firstChild;
	while (nextEntity != ECS::INVALID_ENTITY) {
		auto* child = g_ecs->try_get_component<Instance>(nextEntity);
		if (child)
		{
			if (treeviewEntry.m_numChildren++ > 20 || aDepth > 5)
				treeviewEntry.m_isCollapsed = true;

			RecursiveInit(nextEntity, aDepth + 1);
			child = g_ecs->try_get_component<Instance>(nextEntity);
			nextEntity = child->m_nextChild;
		}
		else
		{
			break;
		}
	}
	g_ecs->add_component<TreeviewEntry>(eInstance, std::move(treeviewEntry));
}

static void RecursiveDraw(ECS::Entity eScreenGui, ECS::Entity eTreeview, ECS::Entity eInstance, size_t aDepth, int& aPosition)
{
	Instance* aInstance = &g_ecs->get_component<Instance>(eInstance);
	if (aInstance->m_name.compare("CoreEditorGui") == 0)
	{
		return;
	}

	TreeviewEntry& treeviewEntry = g_ecs->get_component<TreeviewEntry>(eInstance);
	if (treeviewEntry.m_label == ECS::INVALID_ENTITY)
	{
		std::string sub = "";
		if (aDepth > 0)
		{
			for (int i = 0; i < (aDepth * 4) - 1; ++i)
			{
				sub += (i % 2 == 0) ? '.' : ' ';
			}
		}

		std::string finalName = sub + aInstance->m_name;// std::string{ aInstance->m_name };
		treeviewEntry.m_label = CreateTextLabel({
			.Text{finalName.data()},
			.Font{Game::FontID::MSGOTHIC},
			.FontSize{12u},
			.TextXAlignment{Game::TextXAlignment::LEFT},
			.BorderSize{0},
			.Size{1.f, 0, 0.f, 15},
			.Position{0, 28, 0, aPosition},
			.BackgroundColor{BACKGROUND_COLOR},
			.TextColor{1.f, 1.f, 1.f},
			.OutlineTransparency{1.f},
			.BackgroundTransparency{0.f},
		}, eTreeview);
		aPosition += 16;
	}
	
	aInstance = &g_ecs->get_component<Instance>(eInstance);

	if (treeviewEntry.m_collapseButton == ECS::INVALID_ENTITY)
	{
		treeviewEntry.m_collapseButton = CreateImageButton({
			.Image{"res://ui/open.png"},
			.HoverImage{"res://ui/hover.png"},
			.PressedImage{"res://ui/collapsed.png"},
			.Size{0, 16, 0, 18},
			.Position{0, -18, 0.5f, -9},
			.ImageColor{1.f, 1.f, 1.f},
			.BackgroundTransparency{1.f},
			.ImageTransparency{0.f},
			.Callback{[eInstance, eTreeview, eScreenGui, &aPosition](ECS::Entity) {
				TreeviewEntry& treeviewEntry = g_ecs->get_component<TreeviewEntry>(eInstance);
				treeviewEntry.m_isCollapsed = !treeviewEntry.m_isCollapsed;
				Instance& treeview = g_ecs->get_component<Instance>(eTreeview);
				treeview.m_firstChild = ECS::INVALID_ENTITY;
				treeview.m_lastChild = ECS::INVALID_ENTITY;
				g_ecs->run_system<TreeviewEntry>([&](ECS::Entity, TreeviewEntry& entry) {
					if (entry.m_collapseButton != ECS::INVALID_ENTITY)
					{
						Instance& instance = g_ecs->get_component<Instance>(entry.m_collapseButton);
						ImageButton& button = g_ecs->get_component<ImageButton>(entry.m_collapseButton);
						button.clear(*g_ecs);
						instance.destroy(*g_ecs, entry.m_collapseButton);
						entry.m_collapseButton = ECS::INVALID_ENTITY;
					}
					if (entry.m_label != ECS::INVALID_ENTITY)
					{
						Instance& instance = g_ecs->get_component<Instance>(entry.m_label);
						TextRect& label = g_ecs->get_component<TextRect>(entry.m_label);
						label.clear(*g_ecs);
						instance.destroy(*g_ecs, entry.m_label);
						entry.m_label = ECS::INVALID_ENTITY;
					}
				});
				
				aPosition = 0;
				RecursiveDraw(eScreenGui, eTreeview, g_game->get_self_entity(), 0, aPosition);
				ViewportGui& sGui = g_ecs->get_component<ViewportGui>(eScreenGui);
				Instance& sInstance = g_ecs->get_component<Instance>(eScreenGui);
				sGui.mark_for_redraw(*g_ecs, sInstance);
			}}
		}, treeviewEntry.m_label);
		ImageButton& button = g_ecs->get_component<ImageButton>(treeviewEntry.m_collapseButton);
		Instance& instance = g_ecs->get_component<Instance>(treeviewEntry.m_collapseButton);
		button.set_visible(*g_ecs, instance, treeviewEntry.m_numChildren > 0);
	}

	auto nextEntity = aInstance->m_firstChild;
	while (nextEntity != ECS::INVALID_ENTITY) {
		auto* child = g_ecs->try_get_component<Instance>(nextEntity);
		if (child)
		{
			if (!treeviewEntry.m_isCollapsed)
				RecursiveDraw(eScreenGui, eTreeview, nextEntity, aDepth + 1, aPosition);
			child = g_ecs->try_get_component<Instance>(nextEntity);
			nextEntity = child->m_nextChild;
		}
		else
		{
			break;
		}
	}
}

void EditorFrontend::init() {
	ECS::Entity eCoreGui;
	g_game->get_singleton(InstanceClass::ENGINE_GUI, eCoreGui);

	ECS::Entity eScreenGui;
	auto* instScreenGui = InstanceFactory::create(*g_ecs, InstanceClass::VIEWPORT_GUI, eScreenGui);
	instScreenGui->m_name = "CoreEditorGui";

	CreateVideoRect({
		.Video{"res://test.mp4"},
		.Looped{true},
		.Size{0.f, 512, 0.f, 256},
		.Position{1, -350, 0.f, 0},
		.BackgroundColor{BACKGROUND_COLOR},
		.OutlineColor{1, 0, 0},
		.Transparency{0.f}
	}, eScreenGui);

	CreateVideoRect({
		.Video{"res://test.gif"},
		.Looped{true},
		.Size{0.f, 400, 0.f, 225},
		.Position{0, 150, 1.f, -350},
		.BackgroundColor{BACKGROUND_COLOR},
		.OutlineColor{0, 1, 0},
		.Transparency{0.f}
	}, eScreenGui);

	CreateVideoRect({
		.Video{"res://test.webm"},
		.Looped{true},
		.Size{0.f, 512, 0.f, 256},
		.Position{0.5, -256, 0.5, -128},
		.BackgroundColor{BACKGROUND_COLOR},
		.OutlineColor{0, 0, 1},
		.Transparency{0.f}
	}, eScreenGui);

	auto eTreeview = CreateResizableRect({
		.Size{0.f, 150, 1.f, 0},
		.Position{0, 0, 0.f, 0},
		.BackgroundColor{BACKGROUND_COLOR},
		.OutlineColor{0, 0, 0},
		.MaxSize{550},
		.MinSize{150},
		.Direction{ResizeDirection::RIGHT},
		.Transparency{0.f},
		}, eScreenGui);

	auto eComponents = CreateRect({
		.Size{0.f, 150, 1.f, 0},
		.Position{1, -150, 0.f, 0},
		.BackgroundColor{BACKGROUND_COLOR},
		.OutlineColor{0, 0, 0},
		.Transparency{0.f}
		}, eScreenGui);

	auto eTreeviewContainer = CreateScrollingRect({
		.Size{1.f, 0, 0.85f, 0},
		.Position{0, 8, 0.f, 64},
		.BackgroundColor{DARK_COLOR},
		.HandleWidth{12},
		.Transparency{0.f},
		}, eTreeview);

	RecursiveInit(g_game->get_self_entity());

	CreateTextLabel({
		.Text{"Components"},
		.Font{Game::FontID::ARIAL},
		.FontSize{24u},
		.ZIndex{12},
		.Size{0.f, 150, 0.f, 64},
		.Position{1, -150, 0, 0},
		.BackgroundColor{1.f, 1.f, 1.f},
		.TextColor{1.f, 1.f, 1.f},
		.OutlineTransparency{1.f},
		.BackgroundTransparency{1.f},
	}, eComponents);


	CreateTextLabel({
		.Text{"Treeview"},
		.Font{Game::FontID::ARIAL},
		.FontSize{24u},
		.ZIndex{12},
		.Size{1.f, 0, 0.f, 64},
		.Position{0, 0, 0, 0},
		.BackgroundColor{BACKGROUND_COLOR},
		.TextColor{1.f, 1.f, 1.f},
		.OutlineTransparency{1.f},
		.BackgroundTransparency{0.f},
	}, eTreeview);

	g_ecs->add_component<ECS::Tag<"CoreEditorGui"_hs>>(eScreenGui);
	int posy = 0;
	RecursiveDraw(eScreenGui, eTreeviewContainer, g_game->get_self_entity(), 0, posy);
	instScreenGui = &g_ecs->get_component<Instance>(eScreenGui);
	instScreenGui->set_parent(*g_ecs, eCoreGui, eScreenGui);
}

void EditorFrontend::deinit() {
}

void EditorFrontend::update() {
	g_ecs->run_system<TreeviewEntry>([](ECS::Entity, TreeviewEntry& entry) {
		if (entry.m_collapseButton != ECS::INVALID_ENTITY)
		{
			TextRect& label = g_ecs->get_component<TextRect>(entry.m_label);
			Instance& labelinst = g_ecs->get_component<Instance>(entry.m_label);
			ImageButton& button = g_ecs->get_component<ImageButton>(entry.m_collapseButton);
			Instance& instance = g_ecs->get_component<Instance>(entry.m_collapseButton);
			if (entry.m_isCollapsed)
			{
				label.set_background_color3(*g_ecs, DISABLED_COLOR);
				button.set_image(*g_ecs, instance, "res://ui/collapsed.png");
			}
			else
			{
				if (label.is_mouse_inside())
				{
					label.set_background_color3(*g_ecs, HOVER_COLOR);
				}
				else
				{
					label.set_background_color3(*g_ecs, BACKGROUND_COLOR);
				}
				if (button.is_mouse_inside())
				{
					button.set_image(*g_ecs, instance, "res://ui/hover.png");
				}
				else
				{
					button.set_image(*g_ecs, instance, "res://ui/open.png");
				}
			}
		}
	});
}



