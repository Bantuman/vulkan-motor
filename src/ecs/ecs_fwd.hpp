#pragma once

#include <cstdint>

namespace ECS {
	using Entity = uint32_t;
	using EntityIndex_T = uint32_t;
	using EntityGeneration_T = uint16_t;

	constexpr Entity INVALID_ENTITY = static_cast<Entity>(~0);

	constexpr Entity INDEX_MASK = 0xFFFFF;
	constexpr Entity GENERATION_MASK = 0xFFF;
	constexpr size_t GENERATION_SHIFT = 20;

	constexpr EntityIndex_T get_index(Entity entity) {
		return static_cast<EntityIndex_T>(entity & INDEX_MASK);
	}

	template <uint64_t Value>
	struct Tag {};

	class Manager;
}

