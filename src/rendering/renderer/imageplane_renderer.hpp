#pragma once

#include <core/local.hpp>
#include <core/common.hpp>
#include <core/memory.hpp>

#include <core/geom_type.hpp>
#include <core/normal_id.hpp>

#include <rendering/instance_bucket.hpp>
#include <rendering/image_descriptor_array.hpp>

class Pipeline;
class GeomMesh;
class CommandBuffer;

namespace Game {

struct DecalInstance;

class DecalRenderer {
	public:
		explicit DecalRenderer(VkRenderPass, uint32_t subpass, VkSampleCountFlagBits);

		NULL_COPY_AND_ASSIGN(DecalRenderer);

		void render(CommandBuffer&, VkDescriptorSet);

		DecalInstance& get_or_add_instance(GeomType, NormalId, ECS::Entity);
		void remove_instance(GeomType, NormalId, ECS::Entity);

		uint32_t get_image_index(VkImageView);
	private:
		struct RenderKey {
			GeomType partType;
			NormalId normalId;

			bool operator<(const RenderKey&) const;
			bool operator==(const RenderKey&) const;
		};

		Memory::SharedPtr<Pipeline> m_pipeline;
		InstanceBucketCollection<RenderKey> m_instances;
		Memory::SharedPtr<GeomMesh> m_meshes[static_cast<uint8_t>(Game::GeomType::NUM_TYPES)];
		ImageDescriptorArray m_imageDescriptors;
};

}

inline Local<Game::DecalRenderer> g_decalRenderer;

