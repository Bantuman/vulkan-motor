#include "scene_loader.hpp"

#include <animation/rig.hpp>
#include <animation/animation.hpp>

#include <asset/geom_mesh_cache.hpp>
#include <asset/rigged_mesh_cache.hpp>
#include <asset/animation_cache.hpp>
#include <rendering/render_context.hpp>

#include <core/logging.hpp>

#include <file/file_system.hpp>

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4x4.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Game;

namespace {

struct VertexBoneData {
	uint32_t indices[Rig::MAX_WEIGHTS] = {};
	float weights[Rig::MAX_WEIGHTS] = {};
};

struct RiggedMeshData {
	std::vector<RiggedMesh::Vertex> vertices;
	std::vector<uint32_t> indices;
};

}

/*#include <cstdio>

static void print_matrix(const aiMatrix4x4& m) {
	for (uint32_t y = 0; y < 4; ++y) {
		for (uint32_t x = 0; x < 4; ++x) {
			printf("%.2f\t", m[y][x]);
		}

		putchar('\n');
	}
}*/

static constexpr double TIME_MULTIPLIER = 10000.0;

static void load_rigged_mesh(const aiMesh& mesh, const aiNode& rootNode);
static void load_animation(const aiAnimation& animData);

static Memory::SharedPtr<Rig> create_rig(const aiMesh& mesh, const aiNode& node);
static void calc_rig_hierarchy(RigCreateInfo& createInfo, const aiNode& node,
		const std::string* parentName);
static void calc_vertex_bone_data(Rig& rig, std::vector<VertexBoneData>& boneData,
		const aiMesh& mesh);

bool Asset::load_rigged_mesh_assimp(const std::string_view& fileName) {
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return false;
	}

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(data.data(), data.size(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs
			| aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

	if (!scene) {
		return false;
	}

	LOG_TEMP("%s: %d meshes, %d animations", fileName.data(), scene->mNumMeshes,
			scene->mNumAnimations);

	if (scene->HasMeshes()) {
		for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
			auto* mesh = scene->mMeshes[i];

			if (mesh->HasBones()) {
				load_rigged_mesh(*mesh, *scene->mRootNode);
			}
			// Not loading static meshes for now
		}
	}

	if (scene->HasAnimations()) {
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			load_animation(*scene->mAnimations[i]);
		}
	}

	return true;
}

static void load_rigged_mesh(const aiMesh& mesh, const aiNode& rootNode) {
	RiggedMeshData meshData{};
	std::vector<VertexBoneData> boneData;
	boneData.resize(mesh.mNumVertices);

	auto rig = create_rig(mesh, rootNode);
	calc_vertex_bone_data(*rig, boneData, mesh);

	const aiVector3D aiZeroVector(0.f, 0.f, 0.f);

	for (uint32_t i = 0; i < mesh.mNumVertices; ++i) {
		auto pos = mesh.mVertices[i];
		auto normal = mesh.mNormals[i];
		auto texCoord = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i]
				: aiZeroVector;
		auto tangent = mesh.mTangents[i];
		auto bitangent = mesh.mBitangents[i];

		auto* weights = boneData[i].weights;

		RiggedMesh::Vertex vtx{};
		vtx.position = Math::Vector3(pos.x, pos.y, pos.z);
		vtx.normal = Math::Vector3(normal.x, normal.y, normal.z);
		vtx.tangent = Math::Vector3(tangent.x, tangent.y, tangent.z);
		//vtx.bitangent = Math::Vector3(bitangent.x, bitangent.y, bitangent.z);
		vtx.texCoord = Math::Vector2(texCoord.x, texCoord.y);
		vtx.boneWeights = Math::Vector4(weights[0], weights[1], weights[2], weights[3]);
		memcpy(vtx.boneIndices, boneData[i].indices, sizeof(uint32_t) * Rig::MAX_WEIGHTS);

		meshData.vertices.push_back(std::move(vtx));
	}

	for (uint32_t i = 0; i < mesh.mNumFaces; ++i) {
		const aiFace& face = mesh.mFaces[i];

		meshData.indices.push_back(face.mIndices[0]);
		meshData.indices.push_back(face.mIndices[1]);
		meshData.indices.push_back(face.mIndices[2]);
	}

	auto vertexBuffer = g_renderContext->buffer_create(
			meshData.vertices.size() * sizeof(RiggedMesh::Vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
	auto indexBuffer = g_renderContext->buffer_create(meshData.indices.size() * sizeof(uint32_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

	if (!vertexBuffer || !indexBuffer) {
		return;
	}

	g_renderContext->staging_context_create()
		->add_buffer(*vertexBuffer, meshData.vertices.data())
		.add_buffer(*indexBuffer, meshData.indices.data(), [meshData] {})
		.submit();

	auto riggedMesh = std::make_shared<RiggedMesh>(vertexBuffer, indexBuffer,
			meshData.indices.size(), rig);

	std::string meshName(mesh.mName.C_Str());
	g_riggedMeshCache->set(meshName, riggedMesh);
}

static void load_animation(const aiAnimation& animData) {
	std::string animName(animData.mName.C_Str());
	auto anim = std::make_shared<Animation>(animName);

	for (uint32_t i = 0; i < animData.mNumChannels; ++i) {
		auto& channel = *animData.mChannels[i];
		std::string boneName(channel.mNodeName.C_Str());

		//LOG_TEMP("Loading animation channel for bone %s", boneName.c_str());

		for (uint32_t j = 0; j < channel.mNumRotationKeys; ++j) {
			double dTime = channel.mRotationKeys[j].mTime / animData.mTicksPerSecond;
			//auto iTime = static_cast<int32_t>(dTime * TIME_MULTIPLIER);

			//LOG_TEMP("Frame %d @ time %.2f", j, dTime);
			
			auto& keyPos = channel.mPositionKeys[j].mValue;
			auto& keyRot = channel.mRotationKeys[j].mValue;
			auto& keyScl = channel.mScalingKeys[j].mValue;

			Math::Vector3 pos(keyPos.x, keyPos.y, keyPos.z);
			Math::Quaternion rot(keyRot.w, keyRot.x, keyRot.y, keyRot.z);
			Math::Vector3 scl(keyScl.x, keyScl.y, keyScl.z);

			anim->add_frame(boneName, static_cast<float>(dTime), std::move(pos), std::move(rot),
					std::move(scl));
		}
	}

	g_animationCache->set(animName, std::move(anim));
}

static void add_bone_weight(VertexBoneData& vbd, uint32_t boneIndex, float boneWeight) {
	uint32_t minIndex = 0;
	float minWeight = vbd.weights[0];

	for (uint32_t i = 1; i < Rig::MAX_WEIGHTS; ++i) {
		if (vbd.weights[i] < minWeight) {
			minWeight = vbd.weights[i];
			minIndex = i;
		}
	}

	if (boneWeight > minWeight) {
		vbd.weights[minIndex] = boneWeight;
		vbd.indices[minIndex] = boneIndex;
	}
}

static void calc_vertex_bone_data(Rig& rig, std::vector<VertexBoneData>& boneData,
		const aiMesh& mesh) {
	for (uint32_t i = 0; i < mesh.mNumBones; ++i) {
		auto& bone = *mesh.mBones[i];
		std::string boneName(bone.mName.C_Str());

		auto boneIndex = rig.get_bone_index(boneName);

		for (uint32_t j = 0; j < bone.mNumWeights; ++j) {
			auto& vertexWeight = bone.mWeights[j];
			add_bone_weight(boneData[vertexWeight.mVertexId], boneIndex, vertexWeight.mWeight);
		}
	}
}

static Memory::SharedPtr<Rig> create_rig(const aiMesh& mesh, const aiNode& node) {
	RigCreateInfo createInfo{};

	for (uint32_t i = 0; i < mesh.mNumBones; ++i) {
		auto& bone = *mesh.mBones[i];
		std::string boneName(bone.mName.C_Str());

		Math::Matrix4x4 offsetMatrix{};

		for (uint32_t y = 0; y < 4; ++y) {
			for (uint32_t x = 0; x < 4; ++x) {
				offsetMatrix[y][x] = bone.mOffsetMatrix[x][y];
			}
		}

		//LOG_TEMP("Adding bone %s", boneName.c_str());

		//print_matrix(bone.mOffsetMatrix);

		//createInfo.add_bone(boneName, offsetMatrix, );
	}

	calc_rig_hierarchy(createInfo, node, nullptr);

	return Rig::create(createInfo);
}

static void calc_rig_hierarchy(RigCreateInfo& createInfo, const aiNode& node,
		const std::string* parentName) {
	std::string nodeName(node.mName.C_Str());
	bool isBone = createInfo.get_bone_index(nodeName) != Bone::INVALID_BONE_INDEX;

	//LOG_TEMP("Traversing %s", nodeName.c_str());
	
	if (isBone && !parentName) {
		Math::Matrix4x4 invTF{};

		for (uint32_t y = 0; y < 4; ++y) {
			for (uint32_t x = 0; x < 4; ++x) {
				invTF[y][x] = node.mParent->mTransformation[x][y];
			}
		}

		//createInfo.set_global_inverse_transform(std::move(invTF));
	}

	if (isBone && parentName) {
		//LOG_TEMP("%s.Parent = %s", nodeName.c_str(), parentName->c_str());
		createInfo.set_bone_parent(*parentName, nodeName);
	}

	for (uint32_t i = 0; i < node.mNumChildren; ++i) {
		const auto& child = *node.mChildren[i];

		if (isBone) {
			calc_rig_hierarchy(createInfo, child, &nodeName);
		}
		else {
			calc_rig_hierarchy(createInfo, child, parentName);
		}
	}
}

