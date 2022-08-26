#include "geom_renderer.hpp"

#include <asset/geom_mesh_cache.hpp>
#include <asset/texture_cache.hpp>

#include <core/logging.hpp>

#include <core/geom_instance.hpp>

#include <rendering/cube_map.hpp>
#include <rendering/geom_mesh.hpp>
#include <rendering/render_pipeline.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/render_context.hpp>
#include <rendering/vk_initializers.hpp>

using namespace Game;

PartRenderer::PartRenderer(VkRenderPass normalPass, uint32_t normalSubpass,
			VkRenderPass opaquePass, uint32_t opaqueSubpass,
			VkSampleCountFlagBits opaqueSamples, VkRenderPass transparentPass,
			uint32_t transparentSubpass, Memory::SharedPtr<CubeMap> skybox)
		: m_opaque(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		, m_transparent(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		, m_neededDescriptorUpdates(0)
		, m_skybox(std::move(skybox)) {
	m_meshes[static_cast<uint8_t>(GeomType::BALL)] = g_geomMeshCache->get("Ball");
	m_meshes[static_cast<uint8_t>(GeomType::BLOCK)] = g_geomMeshCache->get("Part");
	m_meshes[static_cast<uint8_t>(GeomType::CYLINDER)] = g_geomMeshCache->get("Cylinder");
	m_meshes[static_cast<uint8_t>(GeomType::WEDGE)] = g_geomMeshCache->get("Wedge");
	m_meshes[static_cast<uint8_t>(GeomType::CORNER_WEDGE)] = g_geomMeshCache->get("CornerWedge");

	auto normalShader = ShaderProgramBuilder()
		.add_shader("shaders://part_depth.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		//.add_shader("shaders://part_normal.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		//.add_shader("shaders://part_normal.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(*g_renderContext);

	auto partShader = ShaderProgramBuilder()
		.add_shader("shaders://tri_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://default_lit.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		//.add_shader("shaders://toon_part.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(*g_renderContext);

	auto transparentShader = ShaderProgramBuilder()
		.add_shader("shaders://tri_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://part_oit.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		//.add_shader("shaders://part_oit_ms.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(*g_renderContext);

	m_normalPipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(GeomMesh::input_attribute_descriptions_part())
		.set_vertex_binding_descriptions(GeomMesh::input_binding_descriptions_part())
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.add_program(*normalShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(normalPass, normalSubpass);

	m_opaquePipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(GeomMesh::input_attribute_descriptions_part())
		.set_vertex_binding_descriptions(GeomMesh::input_binding_descriptions_part())
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_sample_count(opaqueSamples)
		.add_program(*partShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(opaquePass, opaqueSubpass);

	m_transparentPipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(GeomMesh::input_attribute_descriptions_part())
		.set_vertex_binding_descriptions(GeomMesh::input_binding_descriptions_part())
		.set_depth_test_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_blend_enabled(true)
		.set_color_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
		.set_alpha_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
				VK_BLEND_FACTOR_ONE)
		.add_program(*transparentShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(transparentPass, transparentSubpass);

	auto surface = g_textureCache->get_or_load<TextureLoader>("Surface", *g_renderContext,
			"res://surface_diffuse.png", false, true);
	auto surfaceNormal = g_textureCache->get_or_load<TextureLoader>("SurfaceNormal", *g_renderContext,
			"res://surface_normal.png", false, true);

	auto samplerCreateInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, surface->get_num_mip_maps());
	m_sampler = g_renderContext->sampler_create(samplerCreateInfo);

	auto studsInfo = vkinit::descriptor_image_info(*surface->get_image_view(), *m_sampler);
	auto studsNormalInfo = vkinit::descriptor_image_info(*surfaceNormal->get_image_view(),
			*m_sampler);
	auto skyboxInfo = vkinit::descriptor_image_info(*m_skybox->get_image_view(), *m_sampler);

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		m_imageDescriptors[i] = g_renderContext->global_descriptor_set_begin()
			.bind_image(0, studsInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(1, studsNormalInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(2, skyboxInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}
}

void PartRenderer::update() {
	if (m_neededDescriptorUpdates > 0) {
		--m_neededDescriptorUpdates;

		auto imgInfo = vkinit::descriptor_image_info(*m_skybox->get_image_view(), *m_sampler);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imgInfo;
		write.dstBinding = 2;
		write.dstSet = m_imageDescriptors[g_renderContext->get_frame_index()];

		vkUpdateDescriptorSets(g_renderContext->get_device(), 1, &write, 0, nullptr);
	}
}

void PartRenderer::render_pre_pass(CommandBuffer& cmd, VkDescriptorSet globalDescriptor) {
	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_normalPipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_normalPipeline->get_layout(),
			0, 1, &globalDescriptor, 0, nullptr);

	render_internal(cmd, m_opaque);
}

void PartRenderer::render_opaque(CommandBuffer& cmd, VkDescriptorSet globalDescriptor,
		VkDescriptorSet aoDescriptor) {
	VkDescriptorSet dsets[] = {globalDescriptor,
			m_imageDescriptors[g_renderContext->get_frame_index()], aoDescriptor};

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_opaquePipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline->get_layout(),
			0, 3, dsets, 0, nullptr);

	render_internal(cmd, m_opaque);
}

void PartRenderer::render_transparent(CommandBuffer& cmd, VkDescriptorSet globalDescriptor,
		VkDescriptorSet oitDescriptor) {
	VkDescriptorSet dsets[] = {globalDescriptor,
			m_imageDescriptors[g_renderContext->get_frame_index()], /*depthDescriptor,*/
			oitDescriptor};

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_transparentPipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_transparentPipeline->get_layout(), 0, 3, dsets, 0, nullptr);

	render_internal(cmd, m_transparent);
}

void PartRenderer::set_skybox(Memory::SharedPtr<CubeMap> skybox) {
	m_skybox = std::move(skybox);
	m_neededDescriptorUpdates = RenderContext::FRAMES_IN_FLIGHT;
}

GeomInstance& PartRenderer::get_or_add_instance(Game::GeomType partType, bool opaque,
		ECS::Entity entity) {
	if (opaque) {
		return *reinterpret_cast<GeomInstance*>(m_opaque.get_or_add_instance(partType,
					sizeof(GeomInstance), entity));
	}
	else {
		return *reinterpret_cast<GeomInstance*>(m_transparent.get_or_add_instance(partType,
				sizeof(GeomInstance), entity));
	}
}

void PartRenderer::remove_instance(Game::GeomType partType, bool opaque, ECS::Entity entity) {
	if (opaque) {
		m_opaque.remove_instance(partType, entity);
	}
	else {
		m_transparent.remove_instance(partType, entity);
	}
}

void PartRenderer::render_internal(CommandBuffer& cmd,
		InstanceBucketCollection<Game::GeomType>& instances) {
	for (auto& [partType, instData] : instances) {
		if (instData.empty()) {
			continue;
		}

		auto* mesh = m_meshes[static_cast<uint8_t>(partType)].get();

		VkBuffer buffers[] = {*mesh->get_vertex_buffer(), instData.get_buffer()};
		VkDeviceSize offsets[2] = {};

		cmd.bind_vertex_buffers(0, 2, buffers, offsets);
		cmd.bind_index_buffer(*mesh->get_index_buffer(), 0, mesh->get_index_type());
		cmd.draw_indexed(static_cast<uint32_t>(mesh->get_num_indices()), static_cast<uint32_t>(instData.size()), 0, 0, 0);
	}
}

