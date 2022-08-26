#pragma once

#include <volk.h>

class RenderPass;

class CommandBuffer final {
	public:
		explicit CommandBuffer(VkCommandBuffer);
		~CommandBuffer() = default;

		CommandBuffer(CommandBuffer&&);
		CommandBuffer& operator=(CommandBuffer&&);

		CommandBuffer(const CommandBuffer&) = delete;
		void operator=(const CommandBuffer&) = delete;

		void begin(VkCommandBufferUsageFlags flags = 0u);
		void end();
		void reset(VkCommandBufferResetFlags);

		void begin_render_pass(RenderPass& renderPass, VkFramebuffer, uint32_t clearValueCount = 0,
				const VkClearValue* pClearValues = nullptr,
				VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
		void end_render_pass();
		void next_subpass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

		void bind_pipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
		void bind_descriptor_sets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
				uint32_t firstSet, uint32_t descriptorSetCount,
				const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
				const uint32_t* pDynamicOffsets);

		void bind_vertex_buffers(uint32_t firstBinding, uint32_t bindingCount,
				const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
		void bind_index_buffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

		void push_constants(VkPipelineLayout layout, VkShaderStageFlags stageFlags,
				uint32_t offset, uint32_t size, const void* pValues);

		void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
				uint32_t firstInstance);
		void draw_indexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
				int32_t vertexOffset, uint32_t firstInstance);

		void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void pipeline_barrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
				VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
				const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
				const VkBufferMemoryBarrier* pBufferMemoryBarriers,
				uint32_t imageMemoryBarrierCount,
				const VkImageMemoryBarrier* pImageMemoryBarriers);

		void set_viewport(uint32_t firstViewport, uint32_t viewportCount,
				const VkViewport* pViewports);
		void set_scissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);

		void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
				const VkBufferCopy* pRegions);

		void clear_color_image(VkImage image, VkImageLayout imageLayout,
				const VkClearColorValue* pColor, uint32_t rangeCount,
				const VkImageSubresourceRange* pRanges);

		void begin_query(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
		void end_query(VkQueryPool queryPool, uint32_t query);

		void reset_query_pool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);

		void write_timestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
				uint32_t query);

		VkCommandBuffer get_buffer() const;
	private:
		VkCommandBuffer m_cmd;
};

