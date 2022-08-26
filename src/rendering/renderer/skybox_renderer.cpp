#include "skybox_renderer.hpp"

#include <rendering/render_pipeline.hpp>
#include <rendering/cube_map.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/vk_initializers.hpp>

using namespace Game;

SkyboxRenderer::SkyboxRenderer(VkRenderPass renderPass, uint32_t subpass,
			VkSampleCountFlagBits sampleCount, Memory::SharedPtr<CubeMap> skybox)
		: m_skybox(std::move(skybox)) {
	auto skyboxShader = ShaderProgramBuilder()
		.add_shader("shaders://skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(*g_renderContext);

	m_pipeline = PipelineBuilder()
		.add_program(*skyboxShader)
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(false)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_sample_count(sampleCount)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(renderPass, subpass);

	auto samplerCreateInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
	m_sampler = g_renderContext->sampler_create(samplerCreateInfo);

	auto skyboxInfo = vkinit::descriptor_image_info(*m_skybox->get_image_view(), *m_sampler);

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		m_imageDescriptors[i] = g_renderContext->global_descriptor_set_begin()
			.bind_image(0, skyboxInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}
}

void SkyboxRenderer::update() {
	if (m_neededDescriptorUpdates > 0) {
		--m_neededDescriptorUpdates;

		auto imgInfo = vkinit::descriptor_image_info(*m_skybox->get_image_view(), *m_sampler);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imgInfo;
		write.dstBinding = 0;
		write.dstSet = m_imageDescriptors[g_renderContext->get_frame_index()];

		vkUpdateDescriptorSets(g_renderContext->get_device(), 1, &write, 0, nullptr);
	}
}

void SkyboxRenderer::render(CommandBuffer& cmd, VkDescriptorSet globalDescriptor) {
	VkDescriptorSet dsets[] = {globalDescriptor,
			m_imageDescriptors[g_renderContext->get_frame_index()]};

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->get_layout(),
			0, 2, dsets, 0, nullptr);

	cmd.draw(36, 1, 0, 0);
}

void SkyboxRenderer::set_skybox(Memory::SharedPtr<CubeMap> skybox) {
	m_skybox = std::move(skybox);
	m_neededDescriptorUpdates = RenderContext::FRAMES_IN_FLIGHT;
}

