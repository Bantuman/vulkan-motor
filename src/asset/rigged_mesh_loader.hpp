#pragma once

#include <string_view>

#include <ecs/ecs_fwd.hpp>
#include <asset/scene_loader.hpp>

namespace Game::RiggedMeshLoader {

bool load_scene(ECS::Manager& ecs, const std::string_view& filePath, Asset::SceneLoadResults* outResult = nullptr, bool loadAnimations = false);

}

