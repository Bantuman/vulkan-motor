#pragma once

#include <memory>
#include <vector>

#include <core/normal_id.hpp>

#include <math/vector3.hpp>

#include <rendering/mesh.hpp>
#include <rendering/buffer.hpp>

class GeomMesh : public Mesh {
	public:
		struct Vertex {
			Math::Vector3 position;
			Math::Vector3 normal;
			Math::Vector3 tangent;
			Math::Vector3 texCoord;
		};

		static const std::vector<VkVertexInputAttributeDescription>&
				input_attribute_descriptions_part();
		static const std::vector<VkVertexInputBindingDescription>&
				input_binding_descriptions_part();

		static const std::vector<VkVertexInputAttributeDescription>&
				input_attribute_descriptions_decal();
		static const std::vector<VkVertexInputBindingDescription>&
				input_binding_descriptions_decal();

		explicit GeomMesh(std::shared_ptr<Buffer> vertexBuffer,
				std::shared_ptr<Buffer> indexBuffer, size_t numIndices, const size_t* decalOffsets,
				const uint32_t* decalCounts);

		std::shared_ptr<Buffer> get_vertex_buffer() const;
		std::shared_ptr<Buffer> get_index_buffer() const;

		size_t get_num_indices() const;

		size_t get_decal_index_offset(Game::NormalId) const;
		uint32_t get_num_decal_indices(Game::NormalId) const;

		constexpr VkIndexType get_index_type() const {
			return VK_INDEX_TYPE_UINT16;
		}
	private:
		std::shared_ptr<Buffer> m_vertexBuffer;
		std::shared_ptr<Buffer> m_indexBuffer;
		size_t m_numIndices;
		size_t m_decalIndexOffsets[6];
		uint32_t m_decalIndexCounts[6];
};

