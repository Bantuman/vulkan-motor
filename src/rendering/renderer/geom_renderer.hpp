#pragma once

#include <core/local.hpp>
#include <core/common.hpp>
#include <core/memory.hpp>

#include <core/geom_type.hpp>

#include <rendering/geom_mesh.hpp>
#include <rendering/instance_bucket.hpp>
#include <rendering/render_context.hpp>

class Pipeline;
class CubeMap;
class CommandBuffer;

namespace Game {

struct GeomInstance;

class PartRenderer {
	public:
		explicit PartRenderer(VkRenderPass normalPass, uint32_t normalSubpass,
				VkRenderPass opaquePass, uint32_t opaqueSubpass,
				VkSampleCountFlagBits opaqueSamples, VkRenderPass transparentPass,
				uint32_t transparentSubpass, Memory::SharedPtr<CubeMap> skybox);

		NULL_COPY_AND_ASSIGN(PartRenderer);

		void update();

		void render_pre_pass(CommandBuffer&, VkDescriptorSet);
		void render_opaque(CommandBuffer&, VkDescriptorSet globalDescriptor,
				VkDescriptorSet aoDescriptor);
		void render_transparent(CommandBuffer&, VkDescriptorSet globalDescriptor,
				VkDescriptorSet oitDescriptor);

		void set_skybox(Memory::SharedPtr<CubeMap>);

		GeomInstance& get_or_add_instance(Game::GeomType, bool opaque, ECS::Entity);
		void remove_instance(Game::GeomType, bool opaque, ECS::Entity);
	private:
		Memory::SharedPtr<Pipeline> m_normalPipeline;
		Memory::SharedPtr<Pipeline> m_opaquePipeline;
		Memory::SharedPtr<Pipeline> m_transparentPipeline;

		InstanceBucketCollection<Game::GeomType> m_opaque;
		InstanceBucketCollection<Game::GeomType> m_transparent;

		Memory::SharedPtr<GeomMesh> m_meshes[static_cast<uint8_t>(Game::GeomType::NUM_TYPES)];

		VkDescriptorSet m_imageDescriptors[RenderContext::FRAMES_IN_FLIGHT];
		size_t m_neededDescriptorUpdates;

		Memory::SharedPtr<Sampler> m_sampler;
		Memory::SharedPtr<CubeMap> m_skybox;

		void render_internal(CommandBuffer&, InstanceBucketCollection<Game::GeomType>&);
};

}

inline Local<Game::PartRenderer> g_partRenderer;

