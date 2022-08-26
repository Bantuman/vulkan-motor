#pragma once

#include <cstdint>

#include <string>
#include <string_view>
#include <vector>

#include <core/memory.hpp>
#include <core/pair.hpp>

class GeomMesh;
class StaticMesh;
class RiggedMesh;

namespace Game {

class Animation;

}

namespace Asset {

using LoadFlags = uint32_t;

enum LoadFlagBits : uint32_t {
	LOAD_STATIC_MESHES_BIT				= 0b1,
	LOAD_STATIC_MESHES_AS_PARTS_BIT		= 0b10,
	LOAD_RIGGED_MESHES_BIT				= 0b100,
	LOAD_ANIMATIONS_BIT					= 0b1000,
	LOAD_IMAGES_BIT						= 0b10000,

	LOAD_EVERYTHING_BIT					= LOAD_STATIC_MESHES_BIT | LOAD_RIGGED_MESHES_BIT
										  | LOAD_ANIMATIONS_BIT | LOAD_IMAGES_BIT
};

struct SceneLoadResults {
	std::vector<Pair<std::string, Memory::SharedPtr<StaticMesh>>> staticMeshes;
	std::vector<Pair<std::string, Memory::SharedPtr<RiggedMesh>>> riggedMeshes;
	std::vector<Pair<std::string, Memory::SharedPtr<GeomMesh>>> partMeshes;
	std::vector<Pair<std::string, Memory::SharedPtr<Game::Animation>>> animations;
};

bool load_rigged_mesh_assimp(const std::string_view& fileName);
bool load_scene(const std::string_view& filePath, LoadFlags loadFlags,
		SceneLoadResults* loadResults = nullptr);

}

