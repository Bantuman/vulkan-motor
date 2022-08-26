#include "render_pipeline.hpp"

#include <rendering/render_context.hpp>
#include <rendering/shader_program.hpp>

// Pipeline

Pipeline::Pipeline(VkPipeline pipeline, VkPipelineLayout layout)
		: m_pipeline(pipeline)
		, m_layout(layout) {}

Pipeline::~Pipeline() {
	if (m_pipeline != VK_NULL_HANDLE) {
		g_renderContext->queue_delete([pipeline=m_pipeline] {
			vkDestroyPipeline(g_renderContext->get_device(), pipeline, nullptr);
		});
	}
}

Pipeline::Pipeline(Pipeline&& other)
		: m_pipeline(other.m_pipeline)
		, m_layout(other.m_layout) {
	other.m_pipeline = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& other) {
	m_pipeline = other.m_pipeline;
	m_layout = other.m_layout;

	other.m_pipeline = VK_NULL_HANDLE;

	return *this;
}

Pipeline::operator VkPipeline() const {
	return m_pipeline;
}

VkPipeline Pipeline::get_pipeline() const {
	return m_pipeline;
}

VkPipelineLayout Pipeline::get_layout() const {
	return m_layout;
}

// PipelineBuilder

PipelineBuilder::PipelineBuilder()
		: m_vertexInput{}
		, m_inputAssembly{}
		, m_viewportState{}
		, m_rasterizer{}
		, m_multisampling{}
		, m_colorBlendAttachment{}
		, m_colorBlendState{}
		, m_depthStencil{}
		, m_dynamicState{}
		, m_viewport{}
		, m_scissor{} {
	// VertexInputState
	m_vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// InputAssemblyState
	m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// ViewportState
	m_viewport.maxDepth = 1.f;
	
	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.viewportCount = 1;
	m_viewportState.pViewports = &m_viewport;
	m_viewportState.scissorCount = 1;
	m_viewportState.pScissors = &m_scissor;

	// RasterizationState
	m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizer.lineWidth = 1.f;
	m_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	m_rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	// MultisampleState
	m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampling.sampleShadingEnable = VK_FALSE;
	m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampling.minSampleShading = 1.f;
	
	// ColorBlendAttachmentState
	m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colorBlendAttachment.blendEnable = VK_FALSE;
	m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	// ColorBlendState
	m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlendState.logicOpEnable = VK_FALSE;
	m_colorBlendState.attachmentCount = 1;
	m_colorBlendState.pAttachments = &m_colorBlendAttachment;

	// DepthStencilState
	m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencil.depthTestEnable = VK_FALSE;
	m_depthStencil.depthWriteEnable = VK_FALSE;
	m_depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	m_depthStencil.depthBoundsTestEnable = VK_FALSE;
	m_depthStencil.minDepthBounds = 0.f;
	m_depthStencil.maxDepthBounds = 1.f;
	m_depthStencil.stencilTestEnable = VK_FALSE;

	// DynamicState
	m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}

PipelineBuilder& PipelineBuilder::set_vertex_attribute_descriptions(
		const VkVertexInputAttributeDescription* descriptions, size_t count) {
	m_vertexInput.pVertexAttributeDescriptions = descriptions;
	m_vertexInput.vertexAttributeDescriptionCount = count;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_binding_descriptions(
		const VkVertexInputBindingDescription* descriptions, size_t count) {
	m_vertexInput.pVertexBindingDescriptions = descriptions;
	m_vertexInput.vertexBindingDescriptionCount = count;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_viewport_extents(float width, float height) {
	m_viewport.width = width;
	m_viewport.height = height;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_scissor_extents(uint32_t width, uint32_t height) {
	m_scissor.extent = {width, height};

	return *this;
}

PipelineBuilder& PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode) {
	m_rasterizer.cullMode = cullMode;

	return *this;
}

PipelineBuilder& PipelineBuilder::add_program(const ShaderProgram& program) {
	for (uint32_t i = 0; i < program.get_num_modules(); ++i) {
		VkPipelineShaderStageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage = program.get_stage_flags()[i];
		createInfo.module = program.get_shader_modules()[i];
		createInfo.pName = "main";

		m_shaderStages.push_back(std::move(createInfo));
	}

	m_layout = program.get_pipeline_layout();

	return *this;
}

PipelineBuilder& PipelineBuilder::set_blend_enabled(bool enabled) {
	m_colorBlendAttachment.blendEnable = enabled;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_color_blend(VkBlendOp blendOp, VkBlendFactor src,
		VkBlendFactor dst) {
	m_colorBlendAttachment.srcColorBlendFactor = src;
	m_colorBlendAttachment.dstColorBlendFactor = dst;
	m_colorBlendAttachment.colorBlendOp = blendOp;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_alpha_blend(VkBlendOp blendOp, VkBlendFactor src,
		VkBlendFactor dst) {
	m_colorBlendAttachment.srcAlphaBlendFactor = src;
	m_colorBlendAttachment.dstAlphaBlendFactor = dst;
	m_colorBlendAttachment.alphaBlendOp = blendOp;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_test_enabled(bool enabled) {
	m_depthStencil.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_write_enabled(bool enabled) {
	m_depthStencil.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_compare_op(VkCompareOp compareOp) {
	m_depthStencil.depthCompareOp = compareOp;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_sample_count(VkSampleCountFlagBits numSamples) {
	m_multisampling.rasterizationSamples = numSamples;

	return *this;
}

PipelineBuilder& PipelineBuilder::add_dynamic_state(VkDynamicState dynamicState) {
	m_dynamicStates.push_back(dynamicState);

	return *this;
}

std::shared_ptr<Pipeline> PipelineBuilder::build(VkRenderPass renderPass, uint32_t subpassIndex) {
	VkPipeline pipeline;

	VkGraphicsPipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	createInfo.pStages = m_shaderStages.data();
	createInfo.pVertexInputState = &m_vertexInput;
	createInfo.pInputAssemblyState = &m_inputAssembly;
	createInfo.pViewportState = &m_viewportState;
	createInfo.pRasterizationState = &m_rasterizer;
	createInfo.pMultisampleState = &m_multisampling;
	createInfo.pColorBlendState = &m_colorBlendState;
	createInfo.pDepthStencilState = &m_depthStencil;
	createInfo.layout = m_layout;
	createInfo.renderPass = renderPass;
	createInfo.subpass = subpassIndex;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (!m_dynamicStates.empty()) {
		m_dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
		m_dynamicState.pDynamicStates = m_dynamicStates.data();

		createInfo.pDynamicState = &m_dynamicState;
	}

	if (vkCreateGraphicsPipelines(g_renderContext->get_device(), VK_NULL_HANDLE, 1, &createInfo,
			nullptr, &pipeline) != VK_SUCCESS) {
		return {};
	}

	return std::make_shared<Pipeline>(pipeline, m_layout);
}

// ComputePipelineBuilder

ComputePipelineBuilder::ComputePipelineBuilder()
		: m_shaderStage{}
		, m_layout(VK_NULL_HANDLE) {
	m_shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	m_shaderStage.pName = "main";
}

ComputePipelineBuilder& ComputePipelineBuilder::add_program(const ShaderProgram& program) {
	assert(program.get_stage_flags()[0] == VK_SHADER_STAGE_COMPUTE_BIT);

	m_shaderStage.module = program.get_shader_modules()[0];
	m_layout = program.get_pipeline_layout();

	return *this;
}

std::shared_ptr<Pipeline> ComputePipelineBuilder::build() {
	VkPipeline pipeline;

	VkComputePipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.stage = std::move(m_shaderStage);
	createInfo.layout = m_layout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(g_renderContext->get_device(), VK_NULL_HANDLE, 1, &createInfo,
			nullptr, &pipeline) != VK_SUCCESS) {
		return {};
	}

	return std::make_shared<Pipeline>(pipeline, m_layout);
}

