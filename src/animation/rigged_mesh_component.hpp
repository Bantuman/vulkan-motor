#pragma once

#include <vector>

#include <math/transform.hpp>
#include <math/matrix4x4.hpp>

#include <rendering/buffer_component_pool.hpp>

class RiggedMesh;

namespace Game {

struct RiggedMeshInstance : public ECS::GPUBufferObject {
	//Math::Matrix4x4 m_transform;
	Math::Transform m_transform;
};

struct RiggedMeshComponent {
	Memory::SharedPtr<RiggedMesh> m_mesh;
	//Math::Matrix4x4 m_transform;
	std::vector<Math::Matrix4x4> m_finalBoneTransforms;

	uint32_t m_index = static_cast<uint32_t>(~0u);
	uint32_t m_rigInstanceIndex = static_cast<uint32_t>(~0u);
};

}

