#pragma once

#include <rendering/render_context.hpp>
#include <math/vector3.hpp>

class StaticMesh {
	public:
		struct Vertex {
			Math::Vector3 position;
			Math::Vector3 normal;
			Math::Vector3 tangent;
			Math::Vector3 bitangent;
			Math::Vector3 texCoord;
		};

		static const std::vector<VkVertexInputAttributeDescription>&
				input_attribute_descriptions();
		static const std::vector<VkVertexInputBindingDescription>&
				input_binding_descriptions();

		explicit StaticMesh(std::shared_ptr<Buffer> vertexBuffer,
				std::shared_ptr<Buffer> indexBuffer, size_t numIndices);

		std::shared_ptr<Buffer> get_vertex_buffer();
		std::shared_ptr<Buffer> get_index_buffer();
		size_t get_num_indices() const;
	private:
		std::shared_ptr<Buffer> m_vertexBuffer;
		std::shared_ptr<Buffer> m_indexBuffer;
		size_t m_numIndices;
};

struct StaticMeshLoader {
	std::shared_ptr<StaticMesh> load(RenderContext& ctx, const std::string_view& fileName);
};

