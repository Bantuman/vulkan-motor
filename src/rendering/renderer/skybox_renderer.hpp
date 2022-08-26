#pragma once

#include <core/common.hpp>
#include <core/local.hpp>
#include <core/memory.hpp>

#include <rendering/render_context.hpp>

class Pipeline;
class CubeMap;

namespace Game {

class SkyboxRenderer {
	public:
		explicit SkyboxRenderer(VkRenderPass, uint32_t subpass, VkSampleCountFlagBits,
				Memory::SharedPtr<CubeMap> skybox);

		void update();

		void render(CommandBuffer&, VkDescriptorSet globalDescriptor);

		void set_skybox(Memory::SharedPtr<CubeMap>);
	private:
		Memory::SharedPtr<Pipeline> m_pipeline;

		VkDescriptorSet m_imageDescriptors[RenderContext::FRAMES_IN_FLIGHT];
		size_t m_neededDescriptorUpdates;

		Memory::SharedPtr<Sampler> m_sampler;
		Memory::SharedPtr<CubeMap> m_skybox;
};

}

inline Local<Game::SkyboxRenderer> g_skyboxRenderer;

