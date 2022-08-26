#include "imageplane_renderer.hpp"

#include <asset/geom_mesh_cache.hpp>

#include <core/imageplane_instance.hpp>

#include <rendering/geom_mesh.hpp>
#include <rendering/render_pipeline.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/render_context.hpp>
#include <rendering/vk_initializers.hpp>

using namespace Game;

// RenderKey

bool DecalRenderer::RenderKey::operator==(const DecalRenderer::RenderKey& other) const {
	return partType == other.partType && normalId == other.normalId;
}

bool DecalRenderer::RenderKey::operator<(const DecalRenderer::RenderKey& other) const {
	return (partType < other.partType)
			|| (partType == other.partType && normalId < other.normalId);
};

// DecalRenderer

DecalRenderer::DecalRenderer(VkRenderPass renderPass, uint32_t subpass,
			VkSampleCountFlagBits opaqueSamples)
		: m_instances(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		, m_imageDescriptors(32) {
	m_meshes[static_cast<uint8_t>(GeomType::BALL)] = g_geomMeshCache->get("Ball");
	m_meshes[static_cast<uint8_t>(GeomType::BLOCK)] = g_geomMeshCache->get("Part");
	m_meshes[static_cast<uint8_t>(GeomType::CYLINDER)] = g_geomMeshCache->get("Cylinder");
	m_meshes[static_cast<uint8_t>(GeomType::WEDGE)] = g_geomMeshCache->get("Wedge");
	m_meshes[static_cast<uint8_t>(GeomType::CORNER_WEDGE)] = g_geomMeshCache->get("CornerWedge");

	auto decalShader = ShaderProgramBuilder()
			.add_shader("shaders://decals.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
			.add_shader("shaders://decals.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, true)
			.build(*g_renderContext);

	m_pipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(GeomMesh::input_attribute_descriptions_decal())
		.set_vertex_binding_descriptions(GeomMesh::input_binding_descriptions_decal())
		.set_blend_enabled(true)
		.set_color_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
		.set_alpha_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_sample_count(opaqueSamples)
		.add_program(*decalShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(renderPass, subpass);

	auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	auto linearSampler = g_renderContext->sampler_create(samplerInfo);

	m_imageDescriptors.set_sampler(std::move(linearSampler));
}

void DecalRenderer::render(CommandBuffer& cmd, VkDescriptorSet globalDescriptor) {
	m_imageDescriptors.update();
	VkDescriptorSet dsets[] = {globalDescriptor, m_imageDescriptors.get_descriptor_set()};

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->get_layout(),
			0, 2, dsets, 0, nullptr);

	for (auto& [key, instData] : m_instances) {
		if (instData.empty()) {
			continue;
		}

		auto* mesh = m_meshes[static_cast<uint8_t>(key.partType)].get();

		VkBuffer buffers[] = {*mesh->get_vertex_buffer(), instData.get_buffer()};
		VkDeviceSize offsets[2] = {};

		cmd.bind_vertex_buffers(0, 2, buffers, offsets);
		cmd.bind_index_buffer(*mesh->get_index_buffer(),
				mesh->get_decal_index_offset(key.normalId), mesh->get_index_type());
		cmd.draw_indexed(mesh->get_num_decal_indices(key.normalId), static_cast<uint32_t>(instData.size()), 0, 0, 0);
	}
}

DecalInstance& DecalRenderer::get_or_add_instance(GeomType shape, NormalId face,
		ECS::Entity entity) {
	RenderKey key{shape, face};
	return *reinterpret_cast<DecalInstance*>(m_instances.get_or_add_instance(key,
			sizeof(DecalInstance), entity));
}

void DecalRenderer::remove_instance(GeomType shape, NormalId face, ECS::Entity entity) {
	RenderKey key{shape, face};
	m_instances.remove_instance(key, entity);
}

uint32_t DecalRenderer::get_image_index(VkImageView imageView) {
	return m_imageDescriptors.get_image_index(imageView);
}

