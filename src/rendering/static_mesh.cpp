#include "mesh.hpp"
#include "static_mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <file/file_system.hpp>

#include <glm/glm.hpp>

namespace {

struct StaticMeshData {
	std::vector<StaticMesh::Vertex> vertices;
	std::vector<uint32_t> indices;
};

}

static bool load_mesh_data(const std::string_view& fileName, StaticMeshData& resultMesh);

static std::vector<VkVertexInputAttributeDescription> g_staticMeshAttributeDescs = {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMesh::Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMesh::Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMesh::Vertex, tangent)},
	{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMesh::Vertex, bitangent)},
	{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMesh::Vertex, texCoord)}
};

static std::vector<VkVertexInputBindingDescription> g_staticMeshBindingDescs = {
	{0, sizeof(StaticMesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
};

const std::vector<VkVertexInputAttributeDescription>& StaticMesh::input_attribute_descriptions() {
	return g_staticMeshAttributeDescs;
}

const std::vector<VkVertexInputBindingDescription>& StaticMesh::input_binding_descriptions() {
	return g_staticMeshBindingDescs;
}

StaticMesh::StaticMesh(std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer,
			size_t numIndices)
		: m_vertexBuffer(std::move(vertexBuffer))
		, m_indexBuffer(std::move(indexBuffer))
		, m_numIndices(numIndices) {}

std::shared_ptr<Buffer> StaticMesh::get_vertex_buffer() {
	return m_vertexBuffer;
}

std::shared_ptr<Buffer> StaticMesh::get_index_buffer() {
	return m_indexBuffer;
}

size_t StaticMesh::get_num_indices() const {
	return m_numIndices;
}

std::shared_ptr<StaticMesh> StaticMeshLoader::load(RenderContext& ctx,
		const std::string_view& fileName) {
	StaticMeshData meshData{};

	if (!load_mesh_data(fileName, meshData)) {
		return {};
	}

	auto vertexBuffer = ctx.buffer_create(meshData.vertices.size() * sizeof(StaticMesh::Vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
	auto indexBuffer = ctx.buffer_create(meshData.indices.size() * sizeof(uint32_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

	if (!vertexBuffer || !indexBuffer) {
		return {};
	}

	ctx.staging_context_create()
		->add_buffer(*vertexBuffer, meshData.vertices.data())
		.add_buffer(*indexBuffer, meshData.indices.data(), [meshData] {})
		.submit();

	return std::make_shared<StaticMesh>(vertexBuffer, indexBuffer, meshData.indices.size());
}

static bool load_mesh_data(const std::string_view& fileName, StaticMeshData& resultMesh) {
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return false;
	}

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(data.data(), data.size(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs
			| aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

	if (!scene || !scene->HasMeshes()) {
		return false;
	}

	const aiMesh* mesh = scene->mMeshes[0];

	const aiVector3D aiZeroVector(0.f, 0.f, 0.f);

	for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
		const aiVector3D pos = mesh->mVertices[i];
		const aiVector3D normal = mesh->mNormals[i];
		const aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i]
				: aiZeroVector;
		const aiVector3D tangent = mesh->mTangents[i];
		const aiVector3D bitangent = mesh->mBitangents[i];

		resultMesh.vertices.push_back({
			glm::vec3(pos.x, pos.y, pos.z),
			glm::vec3(normal.x, normal.y, normal.z),
			glm::vec3(tangent.x, tangent.y, tangent.z),
			glm::vec3(bitangent.x, bitangent.y, bitangent.z),
			glm::vec3(texCoord.x, texCoord.y, 0)
		});
	}

	constexpr glm::vec3 faceNormals[6] = {
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(-1, 0, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, 0, -1)
	};

	for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];

		resultMesh.indices.push_back(face.mIndices[0]);
		resultMesh.indices.push_back(face.mIndices[1]);
		resultMesh.indices.push_back(face.mIndices[2]);

		glm::vec3 faceNormal{};

		for (uint32_t j = 0; j < 3; ++j) {
			faceNormal += resultMesh.vertices[face.mIndices[j]].normal;
		}

		faceNormal = glm::normalize(faceNormal);
		float maxDot = glm::dot(faceNormal, faceNormals[0]);
		uint8_t maxNormalID = 0;

		for (uint8_t j = 1; j < 6; ++j) {
			float dot = glm::dot(faceNormal, faceNormals[j]);

			if (dot > maxDot) {
				maxDot = dot;
				maxNormalID = j;
			}
		}

		for (uint32_t j = 0; j < 3; ++j) {
			resultMesh.vertices[face.mIndices[j]].texCoord.z = static_cast<float>(maxNormalID);
		}
	}

	return true;
}

