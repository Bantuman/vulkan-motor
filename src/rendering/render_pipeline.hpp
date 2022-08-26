#pragma once

#include <core/common.hpp>

#include <volk.h>

#include <vector>
#include <memory>

class RenderContext;
class ShaderProgram;

class Pipeline {
	public:
		explicit Pipeline(VkPipeline, VkPipelineLayout);
		~Pipeline();

		Pipeline(Pipeline&&);
		Pipeline& operator=(Pipeline&&);

		Pipeline(const Pipeline&) = delete;
		void operator=(const Pipeline&) = delete;

		operator VkPipeline() const;

		VkPipeline get_pipeline() const;
		VkPipelineLayout get_layout() const;
	private:
		VkPipeline m_pipeline;
		VkPipelineLayout m_layout;
};

class PipelineBuilder final {
	public:
		explicit PipelineBuilder();

		NULL_COPY_AND_ASSIGN(PipelineBuilder);

		PipelineBuilder& set_vertex_attribute_descriptions(
				const VkVertexInputAttributeDescription* descriptions, size_t count);
		PipelineBuilder& set_vertex_binding_descriptions(
				const VkVertexInputBindingDescription* descriptions, size_t count);

		template <typename Container>
		PipelineBuilder& set_vertex_attribute_descriptions(const Container& cont) {
			return set_vertex_attribute_descriptions(cont.data(), cont.size());
		}

		template <typename Container>
		PipelineBuilder& set_vertex_binding_descriptions(const Container& cont) {
			return set_vertex_binding_descriptions(cont.data(), cont.size());
		}

		PipelineBuilder& set_viewport_extents(float width, float height);
		PipelineBuilder& set_scissor_extents(uint32_t width, uint32_t height);

		PipelineBuilder& set_cull_mode(VkCullModeFlags);

		PipelineBuilder& add_program(const ShaderProgram&);

		PipelineBuilder& set_blend_enabled(bool);
		PipelineBuilder& set_color_blend(VkBlendOp, VkBlendFactor src, VkBlendFactor dst);
		PipelineBuilder& set_alpha_blend(VkBlendOp, VkBlendFactor src, VkBlendFactor dst);

		PipelineBuilder& set_depth_test_enabled(bool);
		PipelineBuilder& set_depth_write_enabled(bool);
		PipelineBuilder& set_depth_compare_op(VkCompareOp);

		PipelineBuilder& set_sample_count(VkSampleCountFlagBits);

		PipelineBuilder& add_dynamic_state(VkDynamicState);

		[[nodiscard]] std::shared_ptr<Pipeline> build(VkRenderPass, uint32_t subpassIndex);
	private:
		VkPipelineVertexInputStateCreateInfo m_vertexInput;
		VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
		VkPipelineViewportStateCreateInfo m_viewportState;
		VkPipelineRasterizationStateCreateInfo m_rasterizer;
		VkPipelineMultisampleStateCreateInfo m_multisampling;
		VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo m_colorBlendState;
		VkPipelineDepthStencilStateCreateInfo m_depthStencil;
		VkPipelineDynamicStateCreateInfo m_dynamicState;

		VkViewport m_viewport;
		VkRect2D m_scissor;

		VkPipelineLayout m_layout;

		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		std::vector<VkDynamicState> m_dynamicStates;
};

class ComputePipelineBuilder final {
	public:
		explicit ComputePipelineBuilder();

		NULL_COPY_AND_ASSIGN(ComputePipelineBuilder);

		ComputePipelineBuilder& add_program(const ShaderProgram&);

		[[nodiscard]] std::shared_ptr<Pipeline> build();
	private:
		VkPipelineShaderStageCreateInfo m_shaderStage;
		VkPipelineLayout m_layout;
};

