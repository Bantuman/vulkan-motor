#pragma once

#include <cstdint>
#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>

#include <vector>
#include <memory>
#include <rendering/mesh.hpp>
#include <rendering/buffer.hpp>

class Mesh {
	public:
		enum class Type : uint8_t {
			PART,
			RIGGED,
			STATIC,
			NUM_TYPES
		};

		constexpr explicit Mesh(Type type)
				: m_type(type) {}

		constexpr Type get_type() const {
			return m_type;
		}
	private:
		Type m_type;
};

