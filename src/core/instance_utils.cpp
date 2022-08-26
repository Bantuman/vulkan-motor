#include "instance_utils.hpp"

#include <unordered_map>

using namespace Game;

static std::unordered_map<std::string_view, InstanceClass> g_nameToIDMap;

static void try_init_name_to_id_map();

InstanceClass Game::get_instance_class_id_by_name(const std::string_view& name) {
	try_init_name_to_id_map();

	if (auto it = g_nameToIDMap.find(name); it != g_nameToIDMap.end()) {
		return it->second;
	}

	return InstanceClass::INSTANCE;
}

std::string_view Game::get_instance_class_name(InstanceClass id) {
	switch (id) {
		case InstanceClass::INSTANCE:
			return "Instance";
		case InstanceClass::GAME:
			return "Project";
		case InstanceClass::BASE_GEOM:
			return "BaseGeom";
		case InstanceClass::CUBE_GEOM:
			return "Cube";
		case InstanceClass::SLOPE_GEOM:
			return "Slope";
		case InstanceClass::CORNER_SLOPE_GEOM:
			return "CornerSlope";
		case InstanceClass::SPAWN_LOCATION:
			return "Spawn";
		case InstanceClass::MESH_GEOM:
			return "Mesh";
		case InstanceClass::MODEL:
			return "Model";
		case InstanceClass::CAMERA:
			return "Camera";
		case InstanceClass::GAMEWORLD:
			return "Gameworld";
		case InstanceClass::IMAGE_PLANE:
			return "ImagePlane";
		case InstanceClass::AMBIENCE:
			return "Ambience";
		case InstanceClass::SKY:
			return "Sky";
		case InstanceClass::GUI_BASE_2D:
			return "GuiBase2d";
		case InstanceClass::LAYER_COLLECTOR:
			return "LayerCollector";
		case InstanceClass::VIEWPORT_GUI:
			return "ViewportGui";
		case InstanceClass::GUI_OBJECT:
			return "GuiObject";
		case InstanceClass::RECT:
			return "Rect";
		case InstanceClass::TEXT_RECT:
			return "TextRect";
		case InstanceClass::INPUT_RECT:
			return "InputRect";
		case InstanceClass::TEXT_BUTTON:
			return "TextButton";
		case InstanceClass::IMAGE_RECT:
			return "ImageRect";
		case InstanceClass::IMAGE_BUTTON:
			return "ImageButton";
		case InstanceClass::SCROLLING_RECT:
			return "ScrollingRect";
		case InstanceClass::RESIZABLE_RECT:
			return "ResizableRect";
		case InstanceClass::ATTACHMENT:
			return "Attachment";
		case InstanceClass::BONE:
			return "Bone";
		case InstanceClass::ANIMATION_CONTROLLER:
			return "AnimationController";
		case InstanceClass::ANIMATION:
			return "Animation";
		case InstanceClass::PLAYERS:
			return "Players";
		case InstanceClass::ENGINE_GUI:
			return "EngineGui";
		case InstanceClass::GAME_GUI:
			return "GameGui";
		default:
			return "INVALID";
	}
}

bool Game::instance_class_is_a(InstanceClass childToTest, InstanceClass baseClass) {
	switch (baseClass) {
		case InstanceClass::INSTANCE:
			return true;
		case InstanceClass::GUI_OBJECT:
			return childToTest == Game::InstanceClass::RECT
					|| childToTest == Game::InstanceClass::TEXT_RECT
					|| childToTest == Game::InstanceClass::TEXT_BUTTON
					|| childToTest == Game::InstanceClass::INPUT_RECT
					|| childToTest == Game::InstanceClass::IMAGE_RECT
					|| childToTest == Game::InstanceClass::IMAGE_BUTTON
					|| childToTest == Game::InstanceClass::SCROLLING_RECT
					|| childToTest == Game::InstanceClass::RESIZABLE_RECT;
		case InstanceClass::BASE_GEOM:
			return childToTest == InstanceClass::CUBE_GEOM
					|| childToTest == InstanceClass::SLOPE_GEOM
					|| childToTest == InstanceClass::CORNER_SLOPE_GEOM
					|| childToTest == InstanceClass::SPAWN_LOCATION
					|| childToTest == InstanceClass::MESH_GEOM;
		case InstanceClass::ATTACHMENT:
			return childToTest == InstanceClass::ATTACHMENT
					|| childToTest == InstanceClass::BONE;
		default:
			return childToTest == baseClass;
	}
}

bool Game::instance_class_is_creatable(InstanceClass classID) {
	return classID != InstanceClass::GAMEWORLD
		&& classID != InstanceClass::AMBIENCE
		&& classID != InstanceClass::ENGINE_GUI
		&& classID != InstanceClass::GAME_GUI
		&& classID != InstanceClass::PLAYERS;
}

static void try_init_name_to_id_map() {
	if (g_nameToIDMap.empty()) {
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Instance"),
				InstanceClass::INSTANCE));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Project"),
				InstanceClass::GAME));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("BaseGeom"),
				InstanceClass::BASE_GEOM));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Cube"),
				InstanceClass::CUBE_GEOM));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Slope"),
				InstanceClass::SLOPE_GEOM));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("CornerSlope"),
				InstanceClass::CORNER_SLOPE_GEOM));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Spawn"),
				InstanceClass::SPAWN_LOCATION));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Mesh"),
				InstanceClass::MESH_GEOM));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Model"),
				InstanceClass::MODEL));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Attachment"),
				InstanceClass::ATTACHMENT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Bone"),
				InstanceClass::BONE));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("AnimationController"),
				InstanceClass::ANIMATION_CONTROLLER));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Animation"),
				InstanceClass::ANIMATION));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Camera"),
				InstanceClass::CAMERA));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ImagePlane"),
				InstanceClass::IMAGE_PLANE));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("GuiBase2d"),
				InstanceClass::GUI_BASE_2D));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("LayerCollector"),
				InstanceClass::LAYER_COLLECTOR));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ViewportGui"),
				InstanceClass::VIEWPORT_GUI));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("GuiObject"),
				InstanceClass::GUI_OBJECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Rect"),
				InstanceClass::RECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("TextRect"),
				InstanceClass::TEXT_RECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("InputRect"),
				InstanceClass::INPUT_RECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("TextButton"),
				InstanceClass::TEXT_BUTTON));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ImageRect"),
				InstanceClass::IMAGE_RECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ImageButton"),
				InstanceClass::IMAGE_BUTTON));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ScrollingRect"),
				InstanceClass::SCROLLING_RECT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("ResizableRect"),
			InstanceClass::RESIZABLE_RECT));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIGridStyleLayout"),
				InstanceClass::UI_GRID_STYLE_LAYOUT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIGridLayout"),
				InstanceClass::UI_GRID_LAYOUT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIListLayout"),
				InstanceClass::UI_LIST_LAYOUT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIPageLayout"),
				InstanceClass::UI_PAGE_LAYOUT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UITableLayout"),
				InstanceClass::UI_TABLE_LAYOUT));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIPadding"),
				InstanceClass::UI_PADDING));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIScale"),
				InstanceClass::UI_SCALE));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UISizeConstraint"),
				InstanceClass::UI_SIZE_CONSTRAINT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UITextSizeConstraint"),
				InstanceClass::UI_TEXT_SIZE_CONSTRAINT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UICorner"),
				InstanceClass::UI_CORNER));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIGradient"),
				InstanceClass::UI_GRADIENT));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("UIStroke"),
				InstanceClass::UI_STROKE));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Sky"),
				InstanceClass::SKY));

		g_nameToIDMap.emplace(std::make_pair(std::string_view("Gameworld"),
				InstanceClass::GAMEWORLD));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Ambience"),
				InstanceClass::AMBIENCE));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("Players"),
				InstanceClass::PLAYERS));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("EngineGui"),
				InstanceClass::ENGINE_GUI));
		g_nameToIDMap.emplace(std::make_pair(std::string_view("GameGui"),
				InstanceClass::GAME_GUI));
	}
}

