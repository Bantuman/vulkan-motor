#include "rigged_mesh.hpp"

#include <core/mesh_geom_instance.hpp>

static std::vector<VkVertexInputAttributeDescription> g_staticMeshAttributeDescs = {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RiggedMesh::Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RiggedMesh::Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RiggedMesh::Vertex, tangent)},
	{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(RiggedMesh::Vertex, texCoord)},
	{4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(RiggedMesh::Vertex, boneWeights)},
	{5, 0, VK_FORMAT_R32G32B32A32_UINT, offsetof(RiggedMesh::Vertex, boneIndices)},
	// Instance
	{6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::MeshGeomInstance, m_transform)},
	{7, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::MeshGeomInstance, m_transform)
			+ 3 * sizeof(float)},
	{8, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::MeshGeomInstance, m_transform)
			+ 6 * sizeof(float)},
	{9, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Game::MeshGeomInstance, m_transform)
			+ 9 * sizeof(float)},
	{10, 1, VK_FORMAT_R32_SFLOAT, offsetof(Game::MeshGeomInstance, m_reflectance)},
	{11, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::MeshGeomInstance, m_color)},
	{12, 1, VK_FORMAT_R32G32B32_UINT, offsetof(Game::MeshGeomInstance, m_indices)},
};

static std::vector<VkVertexInputBindingDescription> g_staticMeshBindingDescs = {
	{0, sizeof(RiggedMesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	{1, sizeof(Game::MeshGeomInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
};

const std::vector<VkVertexInputAttributeDescription>& RiggedMesh::input_attribute_descriptions() {
	return g_staticMeshAttributeDescs;
}

const std::vector<VkVertexInputBindingDescription>& RiggedMesh::input_binding_descriptions() {
	return g_staticMeshBindingDescs;
}

RiggedMesh::RiggedMesh(std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer,
			size_t numIndices, std::shared_ptr<Game::Rig> rig)
		: Mesh(Mesh::Type::RIGGED)
		, m_vertexBuffer(std::move(vertexBuffer))
		, m_indexBuffer(std::move(indexBuffer))
		, m_numIndices(numIndices)
		, m_rig(std::move(rig)) {}

std::shared_ptr<Buffer> RiggedMesh::get_vertex_buffer() const {
	return m_vertexBuffer;
}

std::shared_ptr<Buffer> RiggedMesh::get_index_buffer() const {
	return m_indexBuffer;
}

size_t RiggedMesh::get_num_indices() const {
	return m_numIndices;
}

std::shared_ptr<Game::Rig> RiggedMesh::get_rig() const {
	return m_rig;
}

