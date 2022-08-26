#pragma once

#include <ecs/ecs.hpp>

#include <core/instance.hpp>

namespace Game {

template <typename GuiType>
inline void on_gui_object_ancestry_changed(ECS::Manager& ecs, Instance& selfInst,
		ECS::Entity selfEntity, Instance*, ECS::Entity) {
	auto& gui = ecs.get_component<GuiType>(selfEntity);

	if (selfInst.find_first_ancestor_of_class(ecs, InstanceClass::VIEWPORT_GUI)
			!= ECS::INVALID_ENTITY
			&& (selfInst.find_first_ancestor_of_class(ecs, InstanceClass::GAME_GUI)
			!= ECS::INVALID_ENTITY
			|| selfInst.find_first_ancestor_of_class(ecs, InstanceClass::ENGINE_GUI))) {
		gui.mark_for_redraw(ecs, selfInst);
	}
	else {
		gui.clear(ecs);
	}
}

}

