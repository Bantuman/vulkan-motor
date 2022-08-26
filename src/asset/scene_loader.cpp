#include "scene_loader.hpp"

#include <animation/rig.hpp>
#include <animation/animation.hpp>

#include <asset/geom_mesh_cache.hpp>
#include <asset/rigged_mesh_cache.hpp>
#include <asset/animation_cache.hpp>

#include <core/logging.hpp>
#include <core/pair.hpp>

#include <file/file_system.hpp>
#include <file/path_utils.hpp>

#include <math/common.hpp>
#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4x4.hpp>
#include <math/transform.hpp>

#include <rendering/render_context.hpp>

#include <tiny_gltf.h>

using namespace Game;

static constexpr double TIME_MULTIPLIER = 10000.0;

static bool find_mesh_skin(const tinygltf::Model& model, size_t meshIndex, size_t& skinIndexOut);

static Memory::SharedPtr<Rig> load_rig(const tinygltf::Model& model, const tinygltf::Skin& skin);
static Pair<std::string, Memory::SharedPtr<RiggedMesh>> load_rigged_mesh(const tinygltf::Model& model,
		const tinygltf::Mesh& mesh, Memory::SharedPtr<Rig> rig);
static Pair<std::string, Memory::SharedPtr<Animation>> load_animation(const tinygltf::Model& model,
		const tinygltf::Animation& animData);

static Pair<std::string, Memory::SharedPtr<GeomMesh>> load_geom_mesh(const tinygltf::Model& model,
		const tinygltf::Mesh& mesh);

bool Asset::load_scene(const std::string_view& fileName, Asset::LoadFlags flags,
		Asset::SceneLoadResults* loadResults) {
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return false;
	}

	tinygltf::Model model{};
	tinygltf::TinyGLTF ctx{};
	std::string errStr;
	std::string warnStr;

	auto ext = PathUtils::get_file_extension(fileName);
	bool result = false;

	if (ext.compare("glb") == 0) {
		result = ctx.LoadBinaryFromMemory(&model, &errStr, &warnStr,
					reinterpret_cast<const unsigned char*>(data.data()), data.size());
	}
	else {
		result = ctx.LoadASCIIFromString(&model, &errStr, &warnStr, data.data(), data.size(), "");
	}

	if (!result) {
		if (!errStr.empty()) {
			LOG_ERROR2("GLTF", errStr.c_str());
		}

		return false;
	}

	if (!warnStr.empty()) {
		LOG_WARNING2("GLTF", warnStr.c_str());
	}

	std::unordered_map<size_t, Memory::SharedPtr<Rig>> skinIndexToRig;

	if (flags & Asset::LOAD_RIGGED_MESHES_BIT) {
		for (size_t i = 0; i < model.skins.size(); ++i) {
			auto& skin = model.skins[i];
			skinIndexToRig.emplace(std::make_pair(i, load_rig(model, skin)));
		}
	}

	for (size_t i = 0; i < model.meshes.size(); ++i) {
		auto& meshData = model.meshes[i];
		size_t skinIndex{};

		if (find_mesh_skin(model, i, skinIndex)) {
			if (flags & Asset::LOAD_RIGGED_MESHES_BIT) {
				auto res = load_rigged_mesh(model, meshData, skinIndexToRig[skinIndex]);

				if (loadResults && res.second) {
					loadResults->riggedMeshes.push_back(std::move(res));
				}
			}
		}
		else {
			if (flags & Asset::LOAD_STATIC_MESHES_BIT) {
				if (flags & Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT) {
					auto res = load_geom_mesh(model, meshData);

					if (loadResults && res.second) {
						loadResults->partMeshes.push_back(std::move(res));
					}
				}
				else {
					//load_static_mesh(model, meshData);
				}
			}
		}
	}

	if (flags & Asset::LOAD_ANIMATIONS_BIT) {
		for (auto& animData : model.animations) {
			auto res = load_animation(model, animData);

			if (loadResults && res.second) {
				loadResults->animations.push_back(std::move(res));
			}
		}
	}

	return true;
}

static void read_node_transform(const tinygltf::Node& node, Math::Matrix4x4& outMatrix) {
	if (!node.matrix.empty()) {
		for (size_t y = 0, i = 0; y < 4; ++y) {
			for (size_t x = 0; x < 4; ++x, ++i) {
				outMatrix[y][x] = static_cast<float>(node.matrix[i]);
			}
		}
	}
	else {
		Math::Vector3 pos{};
		Math::Vector3 scl(1, 1, 1);
		Math::Quaternion rot(1, 0, 0, 0);

		if (!node.translation.empty()) {
			pos = Math::Vector3(node.translation[0], node.translation[1], node.translation[2]);
		}

		if (!node.scale.empty()) {
			scl = Math::Vector3(node.scale[0], node.scale[1], node.scale[2]);
		}

		if (!node.rotation.empty()) {
			rot = Math::Quaternion(node.rotation[3], node.rotation[0], node.rotation[1],
					node.rotation[2]);
		}

		outMatrix = Math::BoneTransform{std::move(pos), std::move(rot), std::move(scl)}.to_matrix();
	}
}

static Memory::SharedPtr<Rig> load_rig(const tinygltf::Model& model, const tinygltf::Skin& skin) {
	RigCreateInfo createInfo{};

	auto& ibAccessor = model.accessors[skin.inverseBindMatrices];
	auto& ibView = model.bufferViews[ibAccessor.bufferView];
	auto& ibBuffer = model.buffers[ibView.buffer];
	// FIXME: Check type and componentType
	auto* ibData = ibBuffer.data.data() + ibView.byteOffset + ibAccessor.byteOffset;
	size_t ibStride = ibView.byteStride != 0 ? ibView.byteStride : (16 * sizeof(float));

	for (auto jointIndex : skin.joints) {
		auto& node = model.nodes[jointIndex];

		Math::Matrix4x4 invBind;
		Math::Matrix4x4 localTransform;

		for (size_t y = 0; y < 4; ++y) {
			for (size_t x = 0; x < 4; ++x) {
				invBind[y][x] = reinterpret_cast<const float*>(ibData)[4 * y + x];
			}
		}

		read_node_transform(node, localTransform);

		createInfo.add_bone(node.name, std::move(invBind), std::move(localTransform));

		ibData += ibStride;
	}

	for (auto jointIndex : skin.joints) {
		auto& parentNode = model.nodes[jointIndex];

		for (auto childIndex : parentNode.children) {
			auto& childNode = model.nodes[childIndex];
			createInfo.set_bone_parent(parentNode.name, childNode.name);
		}
	}

	return Rig::create(createInfo);
}

template <typename ComponentType, typename VertexType>
static void read_mesh_attribute(std::vector<VertexType>& vertices,
		const tinygltf::Model& model, const tinygltf::Primitive& prim, const char* name,
		size_t offset, uint32_t type, uint32_t componentType) {
	auto& accessor = model.accessors[prim.attributes.find(name)->second];
	auto& view = model.bufferViews[accessor.bufferView];
	auto& buffer = model.buffers[view.buffer];
	auto* dataStart = buffer.data.data() + view.byteOffset + accessor.byteOffset;

	size_t dstCompSize = tinygltf::GetComponentSizeInBytes(componentType);
	size_t dstCompCount = tinygltf::GetNumComponentsInType(type);
	size_t dstSize = dstCompSize * dstCompCount;

	size_t srcCompSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t srcCompCount = tinygltf::GetNumComponentsInType(accessor.type);
	size_t srcSize = srcCompSize * srcCompCount;
	size_t stride = view.byteStride == 0 ? srcSize : view.byteStride;

	if (vertices.size() != accessor.count) {
		vertices.resize(accessor.count);
	}

	for (size_t i = 0; i < accessor.count; ++i) {
		auto* dest = reinterpret_cast<uint8_t*>(&vertices[i]) + offset;
		
		if (/*type == static_cast<uint32_t>(accessor.type)
				&&*/ componentType == static_cast<uint32_t>(accessor.componentType)) {
			memcpy(dest, dataStart, Math::min(srcSize, dstSize));
		}
		else {
			memset(dest, 0, dstSize);

			for (size_t j = 0, l = Math::min(srcCompCount, dstCompCount); j < l; ++j) {
				auto* srcCompData = dataStart + srcCompSize * j;
				auto* dstCompData = reinterpret_cast<ComponentType*>(dest + dstCompSize * j);

				switch (static_cast<uint32_t>(accessor.componentType)) {
					case TINYGLTF_COMPONENT_TYPE_BYTE:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const int8_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const uint8_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_SHORT:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const int16_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const uint16_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_INT:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const int32_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const uint32_t*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_FLOAT:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const float*>(srcCompData));
						break;
					case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						*dstCompData = static_cast<ComponentType>(
								*reinterpret_cast<const double*>(srcCompData));
						break;
					default:
						LOG_ERROR("GLTF", "%d is not a valid component type",
								accessor.componentType);
				}
			}
		}

		dataStart += stride;
	}
}

template <typename ComponentType>
static ComponentType* get_mesh_index_buffer(const tinygltf::Model& model,
		const tinygltf::Primitive& prim, uint32_t componentType,
		std::vector<ComponentType>& auxBuffer, size_t& outCount) {
	auto& indexAccessor = model.accessors[prim.indices];
	auto& indexView = model.bufferViews[indexAccessor.bufferView];
	auto& indexBufferObject = model.buffers[indexView.buffer];

	auto* indexDataStart = indexBufferObject.data.data() + indexView.byteOffset
			+ indexAccessor.byteOffset;

	outCount = indexAccessor.count;
	auxBuffer.resize(indexAccessor.count);

	if (static_cast<uint32_t>(indexAccessor.componentType) == componentType) {
		memcpy(auxBuffer.data(), reinterpret_cast<const ComponentType*>(indexDataStart),
			  indexAccessor.count * sizeof(ComponentType));
	}
	else {
		size_t stride = indexView.byteStride == 0
				? tinygltf::GetComponentSizeInBytes(indexAccessor.componentType)
				: indexView.byteStride;

		switch (static_cast<uint32_t>(indexAccessor.componentType)) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				for (size_t i = 0; i < indexAccessor.count; ++i) {
					auxBuffer[i] = static_cast<ComponentType>(
							*reinterpret_cast<const uint16_t*>(indexDataStart));
					indexDataStart += stride;
				}
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				for (size_t i = 0; i < indexAccessor.count; ++i) {
					auxBuffer[i] = static_cast<ComponentType>(
							*reinterpret_cast<const uint32_t*>(indexDataStart));
					indexDataStart += stride;
				}
				break;
			default:
				LOG_ERROR("GLTF", "Unsupported index buffer component type %d",
						indexAccessor.componentType);
		}
	}

	return auxBuffer.data();
}

static Pair<std::string, Memory::SharedPtr<RiggedMesh>> load_rigged_mesh(
		const tinygltf::Model& model, const tinygltf::Mesh& mesh, Memory::SharedPtr<Rig> rig) {
	std::vector<RiggedMesh::Vertex> vertices;

	if (mesh.primitives.size() > 1) {
		LOG_WARNING("Mesh %s has %d primitives", mesh.name.c_str(), mesh.primitives.size());
	}

	auto& prim = mesh.primitives[0];

	std::vector<uint32_t> indexBufferFillData;
	size_t numIndices = 0;
	auto* indexBufferData = get_mesh_index_buffer<uint32_t>(model, prim,
			TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, indexBufferFillData, numIndices);

	read_mesh_attribute<float>(vertices, model, prim, "POSITION", 0, TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "NORMAL",
			offsetof(RiggedMesh::Vertex, normal), TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "TANGENT",
			offsetof(RiggedMesh::Vertex, tangent), TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "TEXCOORD_0",
			offsetof(RiggedMesh::Vertex, texCoord), TINYGLTF_TYPE_VEC2,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "WEIGHTS_0",
			offsetof(RiggedMesh::Vertex, boneWeights), TINYGLTF_TYPE_VEC4,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<uint32_t>(vertices, model, prim, "JOINTS_0",
			offsetof(RiggedMesh::Vertex, boneIndices), TINYGLTF_TYPE_VEC4,
			TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);

	auto vertexBuffer = g_renderContext->buffer_create(
			vertices.size() * sizeof(RiggedMesh::Vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
	auto indexBuffer = g_renderContext->buffer_create(numIndices * sizeof(uint32_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

	if (!vertexBuffer || !indexBuffer) {
		return {};
	}

	g_renderContext->staging_context_create()
		->add_buffer(*vertexBuffer, vertices.data())
		.add_buffer(*indexBuffer, indexBufferData)
		.submit();

	auto riggedMesh = std::make_shared<RiggedMesh>(vertexBuffer, indexBuffer, numIndices,
			std::move(rig));

	g_riggedMeshCache->set(mesh.name, riggedMesh);

	return Pair{mesh.name, std::move(riggedMesh)};
}

static Pair<std::string, Memory::SharedPtr<Animation>> load_animation(const tinygltf::Model& model,
		const tinygltf::Animation& animData) {
	std::unordered_map<std::string,
		std::unordered_map<int32_t, Pair<float, Math::BoneTransform>>> channelData;

	auto anim = std::make_shared<Animation>(animData.name);

	for (auto& channel : animData.channels) {
		auto& targetNode = model.nodes[channel.target_node];
		auto& sampler = animData.samplers[channel.sampler];

		auto& inAccessor = model.accessors[sampler.input];
		auto& inView = model.bufferViews[inAccessor.bufferView];
		auto& inBuffer = model.buffers[inView.buffer];
		auto* inData = inBuffer.data.data() + inView.byteOffset + inAccessor.byteOffset;

		auto& outAccessor = model.accessors[sampler.output];
		auto& outView = model.bufferViews[outAccessor.bufferView];
		auto& outBuffer = model.buffers[outView.buffer];
		auto* outData = outBuffer.data.data() + outView.byteOffset + outAccessor.byteOffset;

		size_t inStride = inView.byteStride;
		size_t outStride = outView.byteStride;

		if (inStride == 0) {
			inStride = sizeof(float);
		}

		if (outStride == 0) {
			if (channel.target_path.compare("rotation") == 0) {
				outStride = 4 * sizeof(float);
			}
			else {
				outStride = 3 * sizeof(float);
			}
		}

		auto& outputData = channelData[targetNode.name];

		for (size_t i = 0; i < inAccessor.count; ++i) {
			auto dTime = *reinterpret_cast<const float*>(inData);
			auto iTime = static_cast<int32_t>(dTime * TIME_MULTIPLIER);

			auto& tfPair = outputData[iTime];
			tfPair.first = dTime;

			if (channel.target_path.compare("translation") == 0) {
				memcpy(&tfPair.second.position, outData, sizeof(Math::Vector3));
			}
			else if (channel.target_path.compare("rotation") == 0) {
				auto* fData = reinterpret_cast<const float*>(outData);
				tfPair.second.rotation = Math::Quaternion(fData[3], fData[0], fData[1], fData[2]);
			}
			else if (channel.target_path.compare("scale") == 0) {
				memcpy(&tfPair.second.scale, outData, sizeof(Math::Vector3));
			}

			inData += inStride;
			outData += outStride;
		}
	}

	for (auto& [channelName, timeMap] : channelData) {
		for (auto& [_, tfPair] : timeMap) {
			anim->add_frame(channelName, tfPair.first, std::move(tfPair.second.position),
					std::move(tfPair.second.rotation), std::move(tfPair.second.scale));
		}
	}

	g_animationCache->set(animData.name, anim);

	return Pair{animData.name, std::move(anim)};
}

static bool find_mesh_skin(const tinygltf::Model& model, size_t meshIndex, size_t& skinIndexOut) {
	for (auto& node : model.nodes) {
		if (node.skin >= 0 && static_cast<size_t>(node.mesh) == meshIndex) {
			skinIndexOut = static_cast<size_t>(node.skin);
			return true;
		}
	}

	return false;
}

static Pair<std::string, Memory::SharedPtr<GeomMesh>> load_geom_mesh(const tinygltf::Model& model,
		const tinygltf::Mesh& mesh) {
	std::vector<GeomMesh::Vertex> vertices;

	if (mesh.primitives.size() > 1) {
		LOG_WARNING("Mesh %s has %d primitives", mesh.name.c_str(), mesh.primitives.size());
	}

	auto& prim = mesh.primitives[0];

	std::vector<uint16_t> indexBufferFillData;
	size_t numIndices = 0;
	auto* indexBufferData = get_mesh_index_buffer<uint16_t>(model, prim,
			TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, indexBufferFillData, numIndices);

	read_mesh_attribute<float>(vertices, model, prim, "POSITION", 0, TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "NORMAL", offsetof(GeomMesh::Vertex, normal),
			TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "TANGENT",
			offsetof(GeomMesh::Vertex, tangent), TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
	read_mesh_attribute<float>(vertices, model, prim, "TEXCOORD_0",
			offsetof(GeomMesh::Vertex, texCoord), TINYGLTF_TYPE_VEC3,
			TINYGLTF_COMPONENT_TYPE_FLOAT);

	constexpr Math::Vector3 faceNormals[6] = {
		Math::Vector3(1, 0, 0),
		Math::Vector3(0, 1, 0),
		Math::Vector3(0, 0, 1),
		Math::Vector3(-1, 0, 0),
		Math::Vector3(0, -1, 0),
		Math::Vector3(0, 0, -1)
	};

	size_t numFaces = numIndices / 3;
	std::vector<uint8_t> faceIDs(numFaces);

	for (size_t i = 0, f = 0; i < numIndices; i += 3, ++f) {
		Math::Vector3 faceNormal{};

		for (size_t j = 0; j < 3; ++j) {
			faceNormal += vertices[indexBufferData[i + j]].normal;
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

		faceIDs[f] = maxNormalID;

		for (size_t j = 0; j < 3; ++j) {
			vertices[indexBufferData[i + j]].texCoord.z = static_cast<float>(maxNormalID);
		}
	}

	for (size_t i = 1; i < numFaces; ++i) {
		size_t j = i;

		while (j > 0 && faceIDs[j] < faceIDs[j - 1]) {
			std::swap(faceIDs[j], faceIDs[j - 1]);

			uint16_t face[3] = {};
			memcpy(face, indexBufferData + 3 * j, 3 * sizeof(uint16_t));
			memcpy(indexBufferData + 3 * j, indexBufferData + 3 * (j - 1), 3 * sizeof(uint16_t));
			memcpy(indexBufferData + 3 * (j - 1), face, 3 * sizeof(uint16_t));

			--j;
		}
	}

	size_t indexOffsets[6] = {};
	uint32_t indexCounts[6] = {};
	uint8_t currNormalID = 0;

	for (size_t f = 0; f < numFaces; ++f) {
		auto normalID = faceIDs[f];

		if (normalID != currNormalID) {
			currNormalID = normalID;
			indexOffsets[normalID] = 3 * sizeof(uint16_t) * f;
		}
	}

	for (uint8_t i = 0; i < 5; ++i) {
		indexCounts[i] = static_cast<uint32_t>((indexOffsets[i + 1] - indexOffsets[i])
				/ sizeof(uint16_t));
	}

	indexCounts[5] = numIndices - static_cast<uint32_t>(indexOffsets[5] / sizeof(uint16_t));

	auto vertexBuffer = g_renderContext->buffer_create(
			vertices.size() * sizeof(GeomMesh::Vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
	auto indexBuffer = g_renderContext->buffer_create(numIndices * sizeof(uint16_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

	if (!vertexBuffer || !indexBuffer) {
		return {};
	}

	g_renderContext->staging_context_create()
		->add_buffer(*vertexBuffer, vertices.data())
		.add_buffer(*indexBuffer, indexBufferData)
		.submit();

	auto partMesh = std::make_shared<GeomMesh>(vertexBuffer, indexBuffer, numIndices,
			indexOffsets, indexCounts);
	g_geomMeshCache->set(mesh.name, partMesh);

	return Pair{mesh.name, std::move(partMesh)};
}

