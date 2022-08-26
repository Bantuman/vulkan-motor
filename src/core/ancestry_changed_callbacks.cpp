#include "ancestry_changed_callbacks.hpp"

#include <core/imageplane.hpp>
#include <core/sky.hpp>

#include <ui/ui_components.hpp>
#include <ui/gui_object_ancestry_changed.hpp>

using namespace Game;

void Game::init_ancestry_changed_callbacks() {
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::RECT)]
			= on_gui_object_ancestry_changed<Rect2D>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::TEXT_RECT)]
			= on_gui_object_ancestry_changed<TextRect>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::TEXT_BUTTON)]
			= on_gui_object_ancestry_changed<TextButton>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::INPUT_RECT)]
			= on_gui_object_ancestry_changed<InputRect>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::IMAGE_RECT)]
			= on_gui_object_ancestry_changed<ImageRect>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::IMAGE_BUTTON)]
			= on_gui_object_ancestry_changed<ImageButton>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::SCROLLING_RECT)]
			= on_gui_object_ancestry_changed<ScrollingRect>;
	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::RESIZABLE_RECT)]
		= on_gui_object_ancestry_changed<ResizableRect>;

	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::IMAGE_PLANE)]
			= ImagePlane::on_ancestry_changed;

	Game::ancestryChangedCallbacks[static_cast<uint32_t>(InstanceClass::SKY)]
			= Sky::on_ancestry_changed;
}

