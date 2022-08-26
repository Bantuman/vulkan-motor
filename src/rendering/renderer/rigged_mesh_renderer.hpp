#pragma once

#include <unordered_map>

#include <core/local.hpp>
#include <core/memory.hpp>

#include <animation/rigged_mesh_component.hpp>

#include <math/matrix4x4.hpp>

#include <rendering/instance_bucket.hpp>
#include <rendering/image_descriptor_array.hpp>

namespace Game {

class Rig;
struct MeshGeomInstance;

}

class RiggedMesh;
class Pipeline;
class CommandBuffer;
struct RenderKey {
	Memory::SharedPtr<Game::Rig> rig;
	Memory::SharedPtr<RiggedMesh> mesh;

	bool operator==(const RenderKey&) const;
	bool operator!=(const RenderKey&) const;
	bool operator<(const RenderKey&) const;
};
class RiggedMeshRenderer {
	
	public:
		explicit RiggedMeshRenderer(VkRenderPass, uint32_t subpassIndex, VkSampleCountFlagBits);

		NULL_COPY_AND_ASSIGN(RiggedMeshRenderer);

		void update();

		void render(CommandBuffer&, VkDescriptorSet);

		Game::MeshGeomInstance& get_or_add_instance(Memory::SharedPtr<RiggedMesh>, bool opaque,
				ECS::Entity);
		void remove_instance(Memory::SharedPtr<RiggedMesh>, bool opaque, ECS::Entity);

		Math::Matrix4x4* get_or_add_rig_instance(const Game::Rig&, ECS::Entity);
		void remove_rig_instance(const Game::Rig&, ECS::Entity);

		uint32_t get_image_index(VkImageView);

		InstanceBucketCollection<RenderKey>& get_instances();

		uint32_t get_default_diffuse_index() const;
		uint32_t get_default_normal_index() const;
	private:


		InstanceBucketCollection<RenderKey> m_instances;
		InstanceBucketCollection<uint32_t> m_rigInstances;

		ImageDescriptorArray m_imageDescriptors;
		uint32_t m_defaultDiffuseIndex;
		uint32_t m_defaultNormalIndex;

		Memory::SharedPtr<Pipeline> m_pipeline;
		Memory::SharedPtr<Pipeline> m_outlinePipeline;

		bool m_needsRigUpdate;

		void render_internal(CommandBuffer&, VkDescriptorSet globalDescriptors,
				VkDescriptorSet imageDescriptors, Pipeline&,
				InstanceBucketCollection<RenderKey>&);
};

inline Local<RiggedMeshRenderer> g_riggedMeshRenderer;

