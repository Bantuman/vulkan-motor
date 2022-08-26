#include "geom_mesh.hpp"

#include <core/geom_instance.hpp>
#include <core/imageplane_instance.hpp>

static std::vector<VkVertexInputAttributeDescription> g_inputAttribDescriptionsPart = {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, tangent)},
	{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, texCoord)},
	{4, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::GeomInstance, m_transform)},
	{5, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::GeomInstance, m_transform)
			+ 3 * sizeof(float)},
	{6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::GeomInstance, m_transform)
			+ 6 * sizeof(float)},
	{7, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::GeomInstance, m_transform)
			+ 9 * sizeof(float)},
	{8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::GeomInstance, m_scale)},
	{9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::GeomInstance, m_color)},
	{10, 1, VK_FORMAT_R32_UINT, offsetof(Game::GeomInstance, m_surfaceTypes)}
};

static std::vector<VkVertexInputBindingDescription> g_inputBindingDescriptionsPart = {
	{0, sizeof(GeomMesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	{1, sizeof(Game::GeomInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
};

static std::vector<VkVertexInputAttributeDescription> g_inputAttribDescriptionsDecal = {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, tangent)},
	{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeomMesh::Vertex, texCoord)},
	{4, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::DecalInstance, m_transform)},
	{5, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::DecalInstance, m_transform)
			+ 3 * sizeof(float)},
	{6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::DecalInstance, m_transform)
			+ 6 * sizeof(float)},
	{7, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::DecalInstance, m_transform)
			+ 9 * sizeof(float)},
	{8, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::DecalInstance, m_scale)},
	{9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::DecalInstance, m_color)},
	{10, 1, VK_FORMAT_R32_UINT, offsetof(Game::DecalInstance, m_imageIndex)}
};

static std::vector<VkVertexInputBindingDescription> g_inputBindingDescriptionsDecal = {
	{0, sizeof(GeomMesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	{1, sizeof(Game::DecalInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
};

const std::vector<VkVertexInputAttributeDescription>&
		GeomMesh::input_attribute_descriptions_part() {
	return g_inputAttribDescriptionsPart;
}

const std::vector<VkVertexInputBindingDescription>& GeomMesh::input_binding_descriptions_part() {
	return g_inputBindingDescriptionsPart;
}

const std::vector<VkVertexInputAttributeDescription>&
		GeomMesh::input_attribute_descriptions_decal() {
	return g_inputAttribDescriptionsDecal;
}

const std::vector<VkVertexInputBindingDescription>& GeomMesh::input_binding_descriptions_decal() {
	return g_inputBindingDescriptionsDecal;
}


GeomMesh::GeomMesh(std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer,
			size_t numIndices, const size_t* decalOffsets, const uint32_t* decalCounts)
		: Mesh(Mesh::Type::PART)
		, m_vertexBuffer(std::move(vertexBuffer))
		, m_indexBuffer(std::move(indexBuffer))
		, m_numIndices(numIndices) {
	memcpy(m_decalIndexOffsets, decalOffsets, 6 * sizeof(size_t));
	memcpy(m_decalIndexCounts, decalCounts, 6 * sizeof(uint32_t));
}

std::shared_ptr<Buffer> GeomMesh::get_vertex_buffer() const {
	return m_vertexBuffer;
}

std::shared_ptr<Buffer> GeomMesh::get_index_buffer() const {
	return m_indexBuffer;
}

size_t GeomMesh::get_num_indices() const {
	return m_numIndices;
}

size_t GeomMesh::get_decal_index_offset(Game::NormalId face) const {
	return m_decalIndexOffsets[static_cast<uint8_t>(face)];
}

uint32_t GeomMesh::get_num_decal_indices(Game::NormalId face) const {
	return m_decalIndexCounts[static_cast<uint8_t>(face)];
}

