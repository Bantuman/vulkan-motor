#include "game_renderer.hpp"

#include <cstdio>

#include <glm/gtx/transform.hpp>

#include <asset/texture_cache.hpp>
#include <asset/cube_map_cache.hpp>
#include <asset/scene_loader.hpp>

#include <core/logging.hpp>
#include <core/application.hpp>

#include <rendering/renderer/ui_renderer.hpp>
#include <rendering/renderer/geom_renderer.hpp>
#include <rendering/renderer/imageplane_renderer.hpp>
#include <rendering/renderer/skybox_renderer.hpp>
#include <rendering/renderer/rigged_mesh_renderer.hpp>

#include <math/common.hpp>
#include <math/matrix_projection.hpp>

#include <rendering/render_pipeline.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/vk_common.hpp>
#include <rendering/vk_initializers.hpp>
#include <rendering/vk_profiler.hpp>
#include <rendering/render_utils.hpp>

static constexpr VkFormat g_depthFormat = VK_FORMAT_D32_SFLOAT;
static constexpr VkFormat g_depthPyramidFormat = VK_FORMAT_R32_SFLOAT;

GameRenderer::GameRenderer(RenderContext& ctx)
		: m_context(&ctx)
		, m_window(g_window.get())
		, m_frames{}
		, m_outFramebuffers(ctx.get_swapchain_image_count())
		, m_sampleCount(VK_SAMPLE_COUNT_4_BIT) {
	m_textBitmap.create();

	Asset::load_scene("res://geom.glb", Asset::LOAD_STATIC_MESHES_BIT
			| Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT);
	Asset::load_scene("res://ball.glb", Asset::LOAD_STATIC_MESHES_BIT
			| Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT);
	Asset::load_scene("res://cylinder.glb", Asset::LOAD_STATIC_MESHES_BIT
			| Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT);
	Asset::load_scene("res://wedge.glb", Asset::LOAD_STATIC_MESHES_BIT
			| Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT);
	Asset::load_scene("res://corner_wedge.glb", Asset::LOAD_STATIC_MESHES_BIT
			| Asset::LOAD_STATIC_MESHES_AS_PARTS_BIT);

	g_textureCache->get_or_load<TextureLoader>("DefaultDiffuseMap", *m_context,
			"res://missing_texture.png", true, false);
	g_textureCache->get_or_load<TextureLoader>("DefaultNormalMap", *m_context,
			"res://flat_normal.png", false, false);

	m_sampler = ctx.sampler_create(vkinit::sampler_create_info(VK_FILTER_LINEAR));

	buffers_init();
	depth_buffer_init();
	depth_pre_pass_init();
	ao_pass_init();
	forward_pass_init();
	output_pass_init();
	frame_data_init();
	renderers_init();

	auto extents = ctx.get_swapchain_extent();
	framebuffers_recreate(extents.width, extents.height);

	ctx.swapchain_resize_event().connect([&](int width, int height) {
		on_swap_chain_resized(width, height);
	});
}

GameRenderer::~GameRenderer() {
	g_uiRenderer.destroy();
	g_decalRenderer.destroy();
	g_riggedMeshRenderer.destroy();
	g_partRenderer.destroy();
	g_skyboxRenderer.destroy();
}

void GameRenderer::render() {
	update_buffers();
	update_descriptors();

	m_context->frame_begin();

	auto cmd = m_context->get_main_command_buffer();

	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>(m_window->get_width());
	viewport.height = static_cast<float>(m_window->get_height());
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = {
		static_cast<uint32_t>(m_window->get_width()),
		static_cast<uint32_t>(m_window->get_height())
	};

	{
		GFX::VulkanScopeTimer frameTimer(*cmd, "Frame");

		cmd->set_viewport(0, 1, &viewport);
		cmd->set_scissor(0, 1, &scissor);

		update_instances(*cmd);

		depth_pre_pass(*cmd);
		reduce_depth(*cmd);
		ao_pass(*cmd);
		blur_ao(*cmd);
		forward_pass(*cmd);
		barrier_oit(*cmd);
		output_pass(*cmd);
	}

	m_context->frame_end();
}

void GameRenderer::update_camera(const Math::Vector3& position, const Math::Matrix4x4& view) {
	m_cameraData.view = view;
	m_cameraData.position = position;

	Math::Vector3 viewSunlightDir(view * Math::Vector4(m_sunlightDirection, 0.f));
	memcpy(&m_sceneData.sunlightDirection, &viewSunlightDir, sizeof(Math::Vector3));
}

void GameRenderer::set_sunlight_dir(const Math::Vector3& dir) {
	m_sunlightDirection = dir;
}

void GameRenderer::set_brightness(float brightness) {
	m_sceneData.sunlightDirection.w = brightness;
}

void GameRenderer::set_skybox(std::shared_ptr<CubeMap> skybox) {
	m_skybox = std::move(skybox);
	g_partRenderer->set_skybox(m_skybox);
	g_skyboxRenderer->set_skybox(m_skybox);
}

RenderContext& GameRenderer::get_context() const {
	return *m_context;
}

TextBitmap& GameRenderer::get_text_bitmap() {
	return *m_textBitmap;
}

// INITIALIZATION METHODS

void GameRenderer::depth_buffer_init() {
	auto extents = m_context->get_swapchain_extent_3d();
	depth_buffer_images_create(extents);

	auto depthReduceShader = ShaderProgramBuilder()
		.add_shader("shaders://depth_reduce.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT)
		.build(*m_context);

	m_depthReducePipeline = ComputePipelineBuilder()
		.add_program(*depthReduceShader)
		.build();

	auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, DEPTH_MIP_COUNT);
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	VkSamplerReductionModeCreateInfo createInfoReduction{};
	createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
	createInfoReduction.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
	samplerInfo.pNext = &createInfoReduction;

	m_depthReduceSampler = m_context->sampler_create(samplerInfo);
}

void GameRenderer::depth_pre_pass_init() {
	m_depthPrePass = RenderPassBuilder()
		.add_attachment(m_depthBuffer, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		.begin_subpass() // Depth Pre-Pass
			.add_depth_stencil_attachment(0)
		.end_subpass()
		.build();
}

void GameRenderer::ao_pass_init() {
	ao_images_create(m_context->get_swapchain_extent_3d());

	m_aoPass = RenderPassBuilder()
		// FIXME: this may be faster to cull w/ the depth buffer and clear to white
		.add_attachment(m_aoImage, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.begin_subpass()
			.add_color_attachment(0)
		.end_subpass()
		.build();

	auto ssaoShader = ShaderProgramBuilder()
		.add_shader("shaders://full_screen_triangle_no_texcoord.vert.spv",
				VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://ssao.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(*m_context);

	auto ssaoBlurShader = ShaderProgramBuilder()
		.add_shader("shaders://ssao_blur.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT)
		.build(*m_context);

	m_ssaoPipeline = PipelineBuilder()
		.add_program(*ssaoShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(*m_aoPass, 0);

	m_ssaoBlurPipeline = ComputePipelineBuilder()
		.add_program(*ssaoBlurShader)
		.build();
}

void GameRenderer::forward_pass_init() {
	auto extents = m_context->get_swapchain_extent_3d();
	forward_images_create(extents);
	oit_images_create(extents);

	m_forwardPass = RenderPassBuilder()
		.add_attachment(m_depthBufferMS, VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		.add_attachment(m_colorAttachment, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.add_attachment(m_aoImage, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.add_attachment(m_depthBuffer, VK_ATTACHMENT_LOAD_OP_LOAD,
				VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		.begin_subpass() // Opaque Pass
			.add_color_attachment(1)
			.add_depth_stencil_attachment(0)
		.next_subpass() // Transparent Pass
			.add_read_only_depth_stencil_attachment(3)
		.end_subpass()
		.build();
}

void GameRenderer::output_pass_init() {
	m_outputPass = RenderPassBuilder()
		.add_swapchain_attachment(VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
		.add_attachment(m_colorAttachment, VK_ATTACHMENT_LOAD_OP_LOAD,
				VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.begin_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.add_color_attachment(0)
			.add_input_attachment(1)
		.next_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.add_color_attachment(0)
		.end_subpass()
		.build();

	auto oitResolveShader = ShaderProgramBuilder()
			.add_shader("shaders://full_screen_triangle_no_texcoord.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
			.add_shader("shaders://oit_resolve.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
			.build(*m_context);

	auto toneMapShader = ShaderProgramBuilder()
			.add_shader("shaders://full_screen_triangle_no_texcoord.vert.spv",
					VK_SHADER_STAGE_VERTEX_BIT)
			.add_shader("shaders://tone_map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
			.build(*m_context);

	m_oitResolvePipeline = PipelineBuilder()
		.set_blend_enabled(true)
		.set_color_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_ALPHA)
		.set_alpha_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO)
		.add_program(*oitResolveShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(*m_outputPass, 0);

	m_toneMapPipeline = PipelineBuilder()
		.add_program(*toneMapShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(*m_outputPass, 0);
}

void GameRenderer::renderers_init() {
	constexpr uint32_t depthPrePassIndex = 0u;
	constexpr uint32_t opaqueIndex = 0u;
	constexpr uint32_t transparentIndex = 1u;

	g_partRenderer.create(*m_depthPrePass, depthPrePassIndex, *m_forwardPass, opaqueIndex,
			m_sampleCount, *m_forwardPass, transparentIndex, m_skybox);
	g_riggedMeshRenderer.create(*m_forwardPass, opaqueIndex, m_sampleCount);
	g_decalRenderer.create(*m_forwardPass, opaqueIndex, m_sampleCount);
	g_skyboxRenderer.create(*m_forwardPass, opaqueIndex, m_sampleCount, m_skybox);
	g_uiRenderer.create(*m_textBitmap, *m_outputPass, 1);
}

void GameRenderer::buffers_init() {
	auto proj = Math::infinite_perspective(glm::radians(70.f),
			static_cast<float>(m_window->get_aspect_ratio()), 0.1f);

	float fWidth = static_cast<float>(m_window->get_width());
	float fHeight = static_cast<float>(m_window->get_height());

	m_cameraData.projInfo = Math::infinite_perspective_clip_to_view_space_deprojection(
			glm::radians(70.f), fWidth, fHeight, 0.1f);

	m_cameraData.projection = std::move(proj);

	m_cameraData.view = Math::Matrix4x4(1.f);

	m_cameraData.screenSize = Math::Vector2(fWidth, fHeight);

	m_window->resize_event().connect([&](int width, int height) {
		auto fWidth = static_cast<float>(width);
		auto fHeight = static_cast<float>(height);

		auto proj = Math::infinite_perspective(glm::radians(70.f),
				static_cast<float>(m_window->get_aspect_ratio()), 0.1f);
		m_cameraData.projInfo = Math::infinite_perspective_clip_to_view_space_deprojection(
				glm::radians(70.f), fWidth, fHeight, 0.1f);

		m_cameraData.projection = std::move(proj);

		m_cameraData.screenSize = Math::Vector2(fWidth, fHeight);

		m_ssaoData.projScale = Math::image_plane_pixels_per_unit(glm::radians(70.f), fHeight);
		memcpy(m_ssaoDataMapping, &m_ssaoData, sizeof(SSAOData));
	});

	m_ssaoData.projScale = Math::image_plane_pixels_per_unit(glm::radians(70.f), fHeight);
	m_ssaoData.bias = 0.01f;
	m_ssaoData.intensity = 1.f;
	m_ssaoData.radius = 1.f;
	m_ssaoData.radius2 = m_ssaoData.radius * m_ssaoData.radius;
	m_ssaoData.intensityDivR6 = m_ssaoData.intensity
			/ (m_ssaoData.radius2 * m_ssaoData.radius2 * m_ssaoData.radius2);

	auto scenePaddedSize = m_context->pad_uniform_buffer_size(sizeof(SceneData));
	auto cameraPaddedSize = m_context->pad_uniform_buffer_size(sizeof(CameraData));

	auto sceneDataBufferSize = RenderContext::FRAMES_IN_FLIGHT * scenePaddedSize;
	auto cameraDataBufferSize = RenderContext::FRAMES_IN_FLIGHT * cameraPaddedSize;

	m_sceneDataBuffer = m_context->buffer_create(sceneDataBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	m_sceneDataMapping = reinterpret_cast<uint8_t*>(m_sceneDataBuffer->map());

	m_cameraDataBuffer = m_context->buffer_create(cameraDataBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	m_cameraDataMapping = reinterpret_cast<uint8_t*>(m_cameraDataBuffer->map());

	m_ssaoDataBuffer = m_context->buffer_create(sizeof(SSAOData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	m_ssaoDataMapping = reinterpret_cast<uint8_t*>(m_ssaoDataBuffer->map());

	memcpy(m_ssaoDataMapping, &m_ssaoData, sizeof(SSAOData));

	// FIXME: this probably belongs in lighting init
	m_sunlightDirection = Math::Vector3(1, 0, 0);

	m_sceneData.ambientColor = Math::Vector4(1, 1, 1, 0.3f);
	m_sceneData.sunlightDirection = Math::Vector4(1, 0, 0, 1);
	m_sceneData.sunlightColor = Math::Vector4(1, 1, 1, 1);

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		memcpy(m_sceneDataMapping + i * scenePaddedSize, &m_sceneData, sizeof(SceneData));
		memcpy(m_cameraDataMapping + i * cameraPaddedSize, &m_cameraData, sizeof(CameraData));
	}

	// FIXME: this probably belongs in lighting init
	//std::string_view skyboxFileNames[] = {
	//	"res://sky512_ft.dds",
	//	"res://sky512_bk.dds",
	//	"res://sky512_up.dds",
	//	"res://sky512_dn.dds",
	//	"res://sky512_rt.dds",
	//	"res://sky512_lf.dds",
	//};


	std::string_view skyboxFileNames[] = {
		"res://space.png",
		"res://space.png",
		"res://space.png",
		"res://space.png",
		"res://space.png",
		"res://space.png",
	};
	m_defaultSkybox = g_cubeMapCache->get_or_load<CubeMapLoader>("DefaultSkybox", *m_context,
			skyboxFileNames, 6, false);
	m_skybox = m_defaultSkybox;
}

void GameRenderer::frame_data_init() {
	auto scenePaddedSize = m_context->pad_uniform_buffer_size(sizeof(SceneData));
	auto cameraPaddedSize = m_context->pad_uniform_buffer_size(sizeof(CameraData));

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		auto& frame = m_frames[i];

		VkDescriptorBufferInfo camBuf{};
		camBuf.buffer = *m_cameraDataBuffer;
		camBuf.offset = i * cameraPaddedSize;
		camBuf.range = sizeof(CameraData);

		VkDescriptorBufferInfo sceneBuf{};
		sceneBuf.buffer = *m_sceneDataBuffer;
		sceneBuf.offset = i * scenePaddedSize;
		sceneBuf.range = sizeof(SceneData);

		auto colorInputInfo = vkinit::descriptor_image_info(*m_viewColorAttachment);
		auto depthInputInfo = vkinit::descriptor_image_info(*m_viewDepthBuffer);

		auto oitColorInfo = vkinit::descriptor_image_info(*m_viewColorBufferOIT, VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_GENERAL);
		auto oitDepthInfo = vkinit::descriptor_image_info(*m_viewDepthBufferOIT, VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_GENERAL);
		auto oitVisInfo = vkinit::descriptor_image_info(*m_viewVisBufferOIT, VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_GENERAL);
		auto oitLockInfo = vkinit::descriptor_image_info(*m_viewLockOIT, VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_GENERAL);

		frame.globalDescriptor = m_context->global_descriptor_set_begin()
			.bind_buffer(0, camBuf, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_buffer(1, sceneBuf, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		frame.depthInputDescriptor = m_context->global_descriptor_set_begin()
			.bind_image(0, depthInputInfo, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		frame.oitWriteDescriptor = m_context->global_descriptor_set_begin()
			.bind_image(0, oitColorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(1, oitDepthInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(2, oitVisInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(3, oitLockInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		frame.oitReadDescriptor = m_context->global_descriptor_set_begin()
			.bind_image(0, oitColorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_image(1, oitVisInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		frame.colorInputDescriptor = m_context->global_descriptor_set_begin()
			.bind_image(0, colorInputInfo, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}
}

// UPDATE METHODS

void GameRenderer::update_buffers() {
	auto scenePaddedSize = m_context->pad_uniform_buffer_size(sizeof(SceneData));
	auto cameraPaddedSize = m_context->pad_uniform_buffer_size(sizeof(CameraData));

	auto frameIndex = m_context->get_frame_index();

	memcpy(m_sceneDataMapping + frameIndex * scenePaddedSize, &m_sceneData, sizeof(SceneData));
	memcpy(m_cameraDataMapping + frameIndex * cameraPaddedSize, &m_cameraData, sizeof(CameraData));
}

void GameRenderer::update_instances(CommandBuffer& cmd) {
	// FIXME: autosync
	cmd.pipeline_barrier(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 0, nullptr);

	g_partRenderer->update();
	g_uiRenderer->update(cmd);
	g_riggedMeshRenderer->update();
	g_skyboxRenderer->update();
}

void GameRenderer::update_descriptors() {
	auto& frame = m_frames[m_context->get_frame_index()];

	if (!frame.needsDescriptorUpdate) {
		return;
	}

	frame.needsDescriptorUpdate = false;

	auto colorInputInfo = vkinit::descriptor_image_info(*m_viewColorAttachment);
	auto depthInputInfo = vkinit::descriptor_image_info(*m_viewDepthBuffer);

	auto oitColorInfo = vkinit::descriptor_image_info(*m_viewColorBufferOIT, VK_NULL_HANDLE,
			VK_IMAGE_LAYOUT_GENERAL);
	auto oitDepthInfo = vkinit::descriptor_image_info(*m_viewDepthBufferOIT, VK_NULL_HANDLE,
			VK_IMAGE_LAYOUT_GENERAL);
	auto oitVisInfo = vkinit::descriptor_image_info(*m_viewVisBufferOIT, VK_NULL_HANDLE,
			VK_IMAGE_LAYOUT_GENERAL);
	auto oitLockInfo = vkinit::descriptor_image_info(*m_viewLockOIT, VK_NULL_HANDLE,
			VK_IMAGE_LAYOUT_GENERAL);

	// FIXME: frame memory
	std::vector<VkWriteDescriptorSet> updateWrites;

	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			frame.colorInputDescriptor, &colorInputInfo, 0));
	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			frame.depthInputDescriptor, &depthInputInfo, 0));

	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitWriteDescriptor, &oitColorInfo, 0));
	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitWriteDescriptor, &oitDepthInfo, 1));
	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitWriteDescriptor, &oitVisInfo, 2));
	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitWriteDescriptor, &oitLockInfo, 3));

	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitReadDescriptor, &oitColorInfo, 0));
	updateWrites.emplace_back(vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			frame.oitReadDescriptor, &oitVisInfo, 1));

	vkUpdateDescriptorSets(m_context->get_device(), static_cast<uint32_t>(updateWrites.size()),
			updateWrites.data(), 0, nullptr);
}

// RENDER METHODS

void GameRenderer::depth_pre_pass(CommandBuffer& cmd) {
	GFX::VulkanScopeTimer timer(cmd, "DepthPrePass");

	VkClearValue clearValues[1] = {};

	// Normal/Depth Pre-Pass
	cmd.begin_render_pass(*m_depthPrePass, *m_depthPrePassFramebuffer, 1, clearValues);
	//cmd.set_viewport(0, 1, &viewport);
	//cmd.set_scissor(0, 1, &scissor);

	auto camOffset = m_context->pad_uniform_buffer_size(sizeof(CameraData))
			* m_context->get_frame_index();

	auto prePassDesc = m_context->dynamic_descriptor_set_begin()
		.bind_buffer(0, {*m_cameraDataBuffer, camOffset, sizeof(CameraData)},
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build();

	g_partRenderer->render_pre_pass(cmd, prePassDesc);

	cmd.end_render_pass();
}

void GameRenderer::ao_pass(CommandBuffer& cmd) {
	GFX::VulkanScopeTimer timer(cmd, "DepthPrePass");

	cmd.begin_render_pass(*m_aoPass, *m_aoFramebuffer);

	auto cameraPaddedSize = m_context->pad_uniform_buffer_size(sizeof(CameraData));

	auto ssaoSet = m_context->dynamic_descriptor_set_begin()
		.bind_buffer(0, {*m_cameraDataBuffer, m_context->get_frame_index() * cameraPaddedSize,
				sizeof(CameraData)}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT)
		.bind_buffer(1, {*m_ssaoDataBuffer, 0, sizeof(SSAOData)},
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.bind_image(2, {*m_depthReduceSampler, *m_viewDepthPyramid,
				VK_IMAGE_LAYOUT_GENERAL},
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_ssaoPipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_ssaoPipeline->get_layout(), 0, 1,
			&ssaoSet, 0, nullptr);

	cmd.draw(3, 1, 0, 0);

	cmd.end_render_pass();
}

void GameRenderer::forward_pass(CommandBuffer& cmd) {
	//GFX::VulkanScopeTimer timer(cmd, "ForwardPass");

	auto& userFrame = m_frames[m_context->get_frame_index()];

	VkClearValue clearValues[2] = {};

	cmd.begin_render_pass(*m_forwardPass, *m_fwdFramebuffer, 1, clearValues);

	auto aoSet = m_context->dynamic_descriptor_set_begin()
		.bind_image(0, {*m_sampler, *m_viewAOImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	// Opaque pass
	{
		GFX::VulkanScopeTimer timer2(cmd, "PartOpaque");
		g_partRenderer->render_opaque(cmd, userFrame.globalDescriptor, aoSet);
	}

	{
		GFX::VulkanScopeTimer timer2(cmd, "RiggedMeshToon");
		g_riggedMeshRenderer->render(cmd, userFrame.globalDescriptor);
	}

	{
		GFX::VulkanScopeTimer timer2(cmd, "Decal");
		g_decalRenderer->render(cmd, userFrame.globalDescriptor);
	}

	{
		GFX::VulkanScopeTimer timer2(cmd, "Skybox");
		g_skyboxRenderer->render(cmd, userFrame.globalDescriptor);
	}

	// Transparent Pass
	cmd.next_subpass();

	{
		GFX::VulkanScopeTimer timer2(cmd, "TransparentPass");

		g_partRenderer->render_transparent(cmd, userFrame.globalDescriptor,
				userFrame.oitWriteDescriptor);
	}

	cmd.end_render_pass();
}

void GameRenderer::blur_ao(CommandBuffer& cmd) {
	GFX::VulkanScopeTimer timer(cmd, "BlurAO");

	auto extents = m_context->get_swapchain_extent();
	int32_t axis[2] = {1, 0};

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *m_ssaoBlurPipeline);

	VkImageMemoryBarrier barriers[2] = {
		vkinit::image_barrier(*m_aoImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT),
		vkinit::image_barrier(*m_aoBlurImage, VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT)
	};

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
			nullptr, 1, &barriers[0]);
	cmd.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
			nullptr, 1, &barriers[1]);

	// blurHorizontal aoImage -> aoBlurImage
	auto hSet = m_context->dynamic_descriptor_set_begin()
		.bind_image(0, {VK_NULL_HANDLE, *m_viewAOImage, VK_IMAGE_LAYOUT_GENERAL},
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.bind_image(1, {VK_NULL_HANDLE, *m_viewAOBlurImage, VK_IMAGE_LAYOUT_GENERAL},
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build();

	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoBlurPipeline->get_layout(), 0,
			1, &hSet, 0, nullptr);

	cmd.push_constants(m_ssaoBlurPipeline->get_layout(), VK_SHADER_STAGE_COMPUTE_BIT,
			0, 2 * sizeof(uint32_t), axis);

	cmd.dispatch(RenderUtils::get_group_count(extents.width, 32),
			RenderUtils::get_group_count(extents.height, 32), 1);

	// barrier
	barriers[0] = vkinit::image_barrier(*m_aoImage, VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT);
	barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
			0, nullptr, 2, barriers);

	axis[0] = 0;
	axis[1] = 1;

	// blurVertical aoBlurImage -> aoImage
	auto vSet = m_context->dynamic_descriptor_set_begin()
		.bind_image(0, {VK_NULL_HANDLE, *m_viewAOBlurImage, VK_IMAGE_LAYOUT_GENERAL},
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.bind_image(1, {VK_NULL_HANDLE, *m_viewAOImage, VK_IMAGE_LAYOUT_GENERAL},
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build();

	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoBlurPipeline->get_layout(), 0,
			1, &vSet, 0, nullptr);

	cmd.push_constants(m_ssaoBlurPipeline->get_layout(), VK_SHADER_STAGE_COMPUTE_BIT,
			0, 2 * sizeof(uint32_t), axis);

	cmd.dispatch(RenderUtils::get_group_count(extents.width, 32),
			RenderUtils::get_group_count(extents.height, 32), 1);

	auto outputBarrier = vkinit::image_barrier(*m_aoImage, VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
			nullptr, 1, &outputBarrier);
}

void GameRenderer::barrier_oit(CommandBuffer& cmd) {
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier barriers[2] = {};

	for (size_t i = 0; i < 2; ++i) {
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barriers[i].subresourceRange = range;
		barriers[i].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	}

	barriers[0].image = *m_colorBufferOIT;
	barriers[1].image = *m_visBufferOIT;

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);
}

void GameRenderer::output_pass(CommandBuffer& cmd) {
	GFX::VulkanScopeTimer timer(cmd, "OutputPass");

	auto& frame = m_frames[m_context->get_frame_index()];

	cmd.begin_render_pass(*m_outputPass,
			*m_outFramebuffers[m_context->get_swapchain_image_index()]);

	// Opaque Tone Map
	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_toneMapPipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_toneMapPipeline->get_layout(),
			0, 1, &frame.colorInputDescriptor, 0, nullptr);

	cmd.draw(3, 1, 0, 0);

	// OIT Resolve
	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_oitResolvePipeline);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_oitResolvePipeline->get_layout(), 0, 1, &frame.oitReadDescriptor, 0, nullptr);

	cmd.draw(3, 1, 0, 0);

	cmd.next_subpass();
	
	g_uiRenderer->render(cmd);

	cmd.end_render_pass();
}

void GameRenderer::reduce_depth(CommandBuffer& cmd) {
	GFX::VulkanScopeTimer timer(cmd, "ReduceDepth");

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *m_depthReducePipeline);

	auto depthBarrier = vkinit::image_barrier(*m_depthBuffer,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1,
			&depthBarrier);

	VkImageMemoryBarrier reduceBarrier{};
	reduceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	reduceBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	reduceBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	reduceBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	reduceBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	reduceBarrier.image = *m_depthPyramid;
	reduceBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	reduceBarrier.subresourceRange.layerCount = 1;
	reduceBarrier.subresourceRange.levelCount = 1;

	VkDescriptorImageInfo dstInfo{};
	dstInfo.sampler = *m_depthReduceSampler;
	dstInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo srcInfo{};
	srcInfo.sampler = *m_depthReduceSampler;

	for (uint32_t i = 0; i < m_depthPyramidLevels; ++i) {
		dstInfo.imageView = *m_viewDepthMips[i];

		if (i == 0) {
			srcInfo.imageView = *m_viewDepthBuffer;
			srcInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else {
			srcInfo.imageView = *m_viewDepthMips[i - 1];
			srcInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}

		auto depthSet = m_context->dynamic_descriptor_set_begin()
			.bind_image(0, dstInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.bind_image(1, srcInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_COMPUTE_BIT)
			.build();

		cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE,
				m_depthReducePipeline->get_layout(), 0, 1, &depthSet, 0, nullptr);

		auto levelWidth = Math::max(m_depthPyramidWidth >> i, 1u);
		auto levelHeight = Math::max(m_depthPyramidHeight >> i, 1u);
		Math::Vector2 imageSize(levelWidth, levelHeight);

		cmd.push_constants(m_depthReducePipeline->get_layout(), VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(imageSize), &imageSize);

		cmd.dispatch(RenderUtils::get_group_count(levelWidth, 32),
				RenderUtils::get_group_count(levelHeight, 32), 1);

		cmd.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1,
				&reduceBarrier);
	}
}

// RECREATE METHODS

void GameRenderer::depth_buffer_images_create(const VkExtent3D& extents) {
	auto imgInfo = vkinit::image_create_info(g_depthFormat, 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT, extents);

	m_depthBuffer = m_context->image_create(imgInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	imgInfo.samples = m_sampleCount;
	m_depthBufferMS = m_context->image_create(imgInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto viewInfo = vkinit::image_view_create_info(VK_IMAGE_VIEW_TYPE_2D, g_depthFormat,
			*m_depthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT);
	auto viewInfoMS = vkinit::image_view_create_info(VK_IMAGE_VIEW_TYPE_2D, g_depthFormat,
			*m_depthBufferMS, VK_IMAGE_ASPECT_DEPTH_BIT);

	m_viewDepthBuffer = m_context->image_view_create(viewInfo);
	m_viewDepthBufferMS = m_context->image_view_create(viewInfoMS);

	m_depthPyramidWidth = extents.width;
	m_depthPyramidHeight = extents.height;
	m_depthPyramidLevels = RenderUtils::get_image_mip_levels(m_depthPyramidWidth,
			m_depthPyramidHeight);

	VkExtent3D pyramidExtent{m_depthPyramidWidth, m_depthPyramidHeight, 1};
	auto pyramidInfo = vkinit::image_create_info(g_depthPyramidFormat, VK_IMAGE_USAGE_SAMPLED_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, pyramidExtent);
	pyramidInfo.mipLevels = m_depthPyramidLevels;

	m_depthPyramid = m_context->image_create(pyramidInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto depthMipViewInfo = vkinit::image_view_create_info(VK_IMAGE_VIEW_TYPE_2D,
			g_depthPyramidFormat, *m_depthPyramid, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	m_viewDepthPyramid = m_context->image_view_create(depthMipViewInfo);

	depthMipViewInfo.subresourceRange.levelCount = 1;

	for (uint32_t i = 0; i < m_depthPyramidLevels; ++i) {
		if (m_viewDepthMips[i]) {
			m_viewDepthMips[i]->delete_late();
		}

		depthMipViewInfo.subresourceRange.baseMipLevel = i;
		m_viewDepthMips[i] = m_context->image_view_create(depthMipViewInfo);
	}

	for (uint32_t i = m_depthPyramidLevels; i < DEPTH_MIP_COUNT; ++i) {
		if (m_viewDepthMips[i]) {
			m_viewDepthMips[i]->delete_late();
			m_viewDepthMips[i] = nullptr;
		}
	}

	m_context->immediate_submit([&](VkCommandBuffer cmd) {
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.image = *m_depthPyramid;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = m_depthPyramidLevels;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
				nullptr, 0, nullptr, 1, &barrier);
	});
}

void GameRenderer::ao_images_create(const VkExtent3D& extents) {
	auto aoInfo = vkinit::image_create_info(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT, extents);

	m_aoImage = m_context->image_create(aoInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewAOImage = m_context->image_view_create(m_aoImage, VK_IMAGE_VIEW_TYPE_2D,
			aoInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);

	m_aoBlurImage = m_context->image_create(aoInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewAOBlurImage = m_context->image_view_create(m_aoBlurImage, VK_IMAGE_VIEW_TYPE_2D,
			aoInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);

	m_context->immediate_submit([&](VkCommandBuffer cmd) {
		auto barrier = vkinit::image_barrier(*m_aoBlurImage, 0,
				VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
				0, nullptr, 1, &barrier);
	});
}

void GameRenderer::forward_images_create(const VkExtent3D& extents) {
	auto colorInfo = vkinit::image_create_info(VK_FORMAT_A2R10G10B10_UNORM_PACK32,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, extents);
	colorInfo.samples = m_sampleCount;

	m_colorAttachment = m_context->image_create(colorInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewColorAttachment = m_context->image_view_create(m_colorAttachment, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_IMAGE_ASPECT_COLOR_BIT);
}

void GameRenderer::oit_images_create(const VkExtent3D& extents) {
	auto colorInfo = vkinit::image_create_info(VK_FORMAT_R32G32B32A32_UINT,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extents);
	m_colorBufferOIT = m_context->image_create(colorInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewColorBufferOIT = m_context->image_view_create(m_colorBufferOIT, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

	auto depthInfo = vkinit::image_create_info(VK_FORMAT_R16G16B16A16_UINT,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extents);
	m_depthBufferOIT = m_context->image_create(depthInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewDepthBufferOIT = m_context->image_view_create(m_depthBufferOIT, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_R16G16B16A16_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

	auto visInfo = vkinit::image_create_info(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extents);
	m_visBufferOIT = m_context->image_create(visInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewVisBufferOIT = m_context->image_view_create(m_visBufferOIT, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	auto lockInfo = vkinit::image_create_info(VK_FORMAT_R32_UINT,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extents);
	m_lockOIT = m_context->image_create(lockInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewLockOIT = m_context->image_view_create(m_lockOIT, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

	m_context->immediate_submit([&](VkCommandBuffer cmd) {
		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier barriers[4] = {};

		for (size_t i = 0; i < 4; ++i) {
			barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barriers[i].subresourceRange = range;
			barriers[i].srcAccessMask = 0;
			barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		}

		barriers[0].image = *m_colorBufferOIT;
		barriers[1].image = *m_depthBufferOIT;
		barriers[2].image = *m_visBufferOIT;
		barriers[3].image = *m_lockOIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
				4, barriers);

		VkClearColorValue clearColorValue{};
		clearColorValue.float32[0] = 1.f;
		clearColorValue.float32[1] = 1.f;
		clearColorValue.float32[2] = 1.f;
		clearColorValue.float32[3] = 1.f;

		vkCmdClearColorImage(cmd, *m_visBufferOIT, VK_IMAGE_LAYOUT_GENERAL, &clearColorValue, 1,
				&range);

		clearColorValue.uint32[0] = 0u;
		vkCmdClearColorImage(cmd, *m_lockOIT, VK_IMAGE_LAYOUT_GENERAL, &clearColorValue, 1,
				&range);
	});
}

void GameRenderer::depth_buffer_recreate(const VkExtent3D& extents) {
	m_depthBufferMS->delete_late();
	m_viewDepthBufferMS->delete_late();

	m_depthBuffer->delete_late();
	m_viewDepthBuffer->delete_late();

	m_depthPyramid->delete_late();
	m_viewDepthPyramid->delete_late();

	depth_buffer_images_create(extents);
}

void GameRenderer::ao_pass_recreate(const VkExtent3D& extents) {
	m_aoImage->delete_late();
	m_viewAOImage->delete_late();

	m_aoBlurImage->delete_late();
	m_viewAOBlurImage->delete_late();

	ao_images_create(extents);
}

void GameRenderer::forward_pass_recreate(const VkExtent3D& extents) {
	m_colorAttachment->delete_late();
	m_viewColorAttachment->delete_late();

	forward_images_create(extents);
}

void GameRenderer::oit_pass_recreate(const VkExtent3D& extents) {
	m_colorBufferOIT->delete_late();
	m_viewColorBufferOIT->delete_late();

	m_depthBufferOIT->delete_late();
	m_viewDepthBufferOIT->delete_late();

	m_visBufferOIT->delete_late();
	m_viewVisBufferOIT->delete_late();

	m_lockOIT->delete_late();
	m_viewLockOIT->delete_late();

	oit_images_create(extents);
}

void GameRenderer::framebuffers_recreate(uint32_t width, uint32_t height) {
	VkImageView fwdAttachments[] = {*m_viewDepthBufferMS, *m_viewColorAttachment,
			*m_viewAOImage, *m_viewDepthBuffer};
	VkImageView preAttachments[] = {*m_viewDepthBuffer};
	VkImageView aoAttachments[] = {*m_viewAOImage};

	m_depthPrePassFramebuffer = m_context->framebuffer_create(*m_depthPrePass, 1, preAttachments,
			width, height);

	m_aoFramebuffer = m_context->framebuffer_create(*m_aoPass, 1, aoAttachments, width, height);

	m_fwdFramebuffer = m_context->framebuffer_create(*m_forwardPass, 4, fwdAttachments, width,
			height);

	VkImageView outAttachments[] = {VK_NULL_HANDLE, *m_viewColorAttachment};

	for (uint32_t i = 0; i < m_context->get_swapchain_image_count(); ++i) {
		outAttachments[0] = m_context->get_swapchain_image_view(i);
		m_outFramebuffers[i] = m_context->framebuffer_create(*m_outputPass, 2, outAttachments,
				width, height);
	}
}

void GameRenderer::on_swap_chain_resized(int width, int height) {
	if (width == 0 || height == 0) {
		return;
	}

	VkExtent3D extents = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		m_frames[i].needsDescriptorUpdate = true;
	}

	depth_buffer_recreate(extents);
	ao_pass_recreate(extents);
	forward_pass_recreate(extents);
	oit_pass_recreate(extents);

	framebuffers_recreate(extents.width, extents.height);
}

