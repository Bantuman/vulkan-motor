#pragma once

#include <cstdint>

namespace ECS {

class TypeIDGenerator {
	private:
		inline static uint64_t s_counter{};

		template <typename T>
		inline static const uint64_t s_value = s_counter++;
	public:
		template <typename T>
		static uint64_t get_type_id() {
			return s_value<T>;
		}
};

}

