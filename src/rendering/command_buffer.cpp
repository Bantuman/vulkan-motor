#include "command_buffer.hpp"

#include <rendering/vk_common.hpp>
#include <rendering/vk_initializers.hpp>
#include <rendering/render_context.hpp>
#include <rendering/render_pass.hpp>

CommandBuffer::CommandBuffer(VkCommandBuffer cmd)
		: m_cmd(cmd) {}

CommandBuffer::CommandBuffer(CommandBuffer&& other)
		: m_cmd(other.m_cmd) {
	other.m_cmd = VK_NULL_HANDLE;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) {
	m_cmd = other.m_cmd;
	other.m_cmd = VK_NULL_HANDLE;

	return *this;
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
	auto beginInfo = vkinit::command_buffer_begin_info(flags);
	VK_CHECK(vkBeginCommandBuffer(m_cmd, &beginInfo));
}

void CommandBuffer::end() {
	VK_CHECK(vkEndCommandBuffer(m_cmd));
}

void CommandBuffer::reset(VkCommandBufferResetFlags flags) {
	VK_CHECK(vkResetCommandBuffer(m_cmd, flags));
}

void CommandBuffer::begin_render_pass(RenderPass& renderPass, VkFramebuffer framebuffer,
		uint32_t clearValueCount, const VkClearValue* pClearValues, VkSubpassContents contents) {
	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = renderPass;
	beginInfo.framebuffer = framebuffer;
	beginInfo.renderArea.extent = g_renderContext->get_swapchain_extent(); // FIXME 
	beginInfo.clearValueCount = clearValueCount;
	beginInfo.pClearValues = pClearValues;

	vkCmdBeginRenderPass(m_cmd, &beginInfo, contents);
}

void CommandBuffer::end_render_pass() {
	vkCmdEndRenderPass(m_cmd);
}

void CommandBuffer::next_subpass(VkSubpassContents contents) {
	vkCmdNextSubpass(m_cmd, contents);
}

void CommandBuffer::bind_pipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
	vkCmdBindPipeline(m_cmd, pipelineBindPoint, pipeline);
}

void CommandBuffer::bind_descriptor_sets(VkPipelineBindPoint pipelineBindPoint,
		VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
		const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
		const uint32_t *pDynamicOffsets) {
	vkCmdBindDescriptorSets(m_cmd, pipelineBindPoint, layout, firstSet, descriptorSetCount,
			pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void CommandBuffer::bind_vertex_buffers(uint32_t firstBinding, uint32_t bindingCount,
		const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) {
	vkCmdBindVertexBuffers(m_cmd, firstBinding, bindingCount, pBuffers, pOffsets);
}

void CommandBuffer::bind_index_buffer(VkBuffer buffer, VkDeviceSize offset,
		VkIndexType indexType) {
	vkCmdBindIndexBuffer(m_cmd, buffer, offset, indexType);
}

void CommandBuffer::push_constants(VkPipelineLayout layout, VkShaderStageFlags stageFlags,
		uint32_t offset, uint32_t size, const void *pValues) {
	vkCmdPushConstants(m_cmd, layout, stageFlags, offset, size, pValues);
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
		uint32_t firstInstance) {
	vkCmdDraw(m_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::draw_indexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
		int32_t vertexOffset, uint32_t firstInstance) {
	vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
	vkCmdDispatch(m_cmd, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::pipeline_barrier(VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
		uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
		uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
		uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
	vkCmdPipelineBarrier(m_cmd, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
			pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
			imageMemoryBarrierCount, pImageMemoryBarriers);
}

void CommandBuffer::set_viewport(uint32_t firstViewport, uint32_t viewportCount,
		const VkViewport *pViewports) {
	vkCmdSetViewport(m_cmd, firstViewport, viewportCount, pViewports);
}

void CommandBuffer::set_scissor(uint32_t firstScissor, uint32_t scissorCount,
		const VkRect2D *pScissors) {
	vkCmdSetScissor(m_cmd, firstScissor, scissorCount, pScissors);
}

void CommandBuffer::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
		const VkBufferCopy *pRegions) {
	vkCmdCopyBuffer(m_cmd, srcBuffer, dstBuffer, regionCount, pRegions);
}

void CommandBuffer::clear_color_image(VkImage image, VkImageLayout imageLayout,
		const VkClearColorValue *pColor, uint32_t rangeCount,
		const VkImageSubresourceRange *pRanges) {
	vkCmdClearColorImage(m_cmd, image, imageLayout, pColor, rangeCount, pRanges);
}

void CommandBuffer::begin_query(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
	vkCmdBeginQuery(m_cmd, queryPool, query, flags);
}

void CommandBuffer::end_query(VkQueryPool queryPool, uint32_t query) {
	vkCmdEndQuery(m_cmd, queryPool, query);
}

void CommandBuffer::reset_query_pool(VkQueryPool queryPool, uint32_t firstQuery,
		uint32_t queryCount) {
	vkCmdResetQueryPool(m_cmd, queryPool, firstQuery, queryCount);
}

void CommandBuffer::write_timestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
		uint32_t query) {
	vkCmdWriteTimestamp(m_cmd, pipelineStage, queryPool, query);
}

VkCommandBuffer CommandBuffer::get_buffer() const {
	return m_cmd;
}

