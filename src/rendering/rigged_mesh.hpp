#pragma once

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>

#include <vector>
#include <memory>
#include <rendering/mesh.hpp>
#include <rendering/buffer.hpp>

namespace Game {

class Rig;

}

class RiggedMesh : public Mesh {
	public:
		struct Vertex {
			Math::Vector3 position;
			Math::Vector3 normal;
			Math::Vector3 tangent;
			Math::Vector2 texCoord;
			Math::Vector4 boneWeights;
			uint32_t boneIndices[4];
		};

		static const std::vector<VkVertexInputAttributeDescription>&
				input_attribute_descriptions();
		static const std::vector<VkVertexInputBindingDescription>&
				input_binding_descriptions();

		explicit RiggedMesh(std::shared_ptr<Buffer> vertexBuffer,
				std::shared_ptr<Buffer> indexBuffer, size_t numIndices,
				std::shared_ptr<Game::Rig> rig);

		std::shared_ptr<Buffer> get_vertex_buffer() const;
		std::shared_ptr<Buffer> get_index_buffer() const;
		size_t get_num_indices() const;
		std::shared_ptr<Game::Rig> get_rig() const;
	private:
		std::shared_ptr<Buffer> m_vertexBuffer;
		std::shared_ptr<Buffer> m_indexBuffer;
		size_t m_numIndices;
		std::shared_ptr<Game::Rig> m_rig;
};

