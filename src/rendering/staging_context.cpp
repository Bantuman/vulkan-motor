#include "staging_context.hpp"

#include <rendering/render_context.hpp>
#include <rendering/vk_common.hpp>
#include <rendering/vk_initializers.hpp>

#include <cstring>

// STAGING CONTEXT

StagingContext& StagingContext::add_buffer(Buffer& buffer, const void* inputMemory,
		DeletorFunction deletor) {
	stagedObjects.push_back({
		inputMemory,
		buffer.get_size(),
		STAGING_TYPE_BUFFER,
		deletor,
		{buffer.get_buffer()}
	});

	return *this;
}

StagingContext& StagingContext::add_image(Image& image, const void* inputMemory, size_t inputSize,
		const VkImageSubresourceRange& range, VkImageLayout finalLayout,
		VkAccessFlagBits finalAccess, DeletorFunction deletor) {
	StagingInfo info;
	info.inputMemory = inputMemory;
	info.inputSize = inputSize;
	info.type = STAGING_TYPE_IMAGE;
	info.deletor = deletor;

	info.image.image = image.get_image();
	info.image.extent = image.get_extent();
	info.image.range = range;
	info.image.finalLayout = finalLayout;
	info.image.finalAccess = finalAccess;

	stagedObjects.push_back(std::move(info));

	return *this;
}

void StagingContext::submit() {
	size_t stagingBufferSize = 0;

	for (auto& staged : stagedObjects) {
		stagingBufferSize += staged.inputSize;
	}

	auto stagingBuffer = context->buffer_create(stagingBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(context->get_device(), &allocInfo, &cmd));

	VkCommandBuffer graphicsCmd = context->get_graphics_upload_command_buffer();

	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
	VK_CHECK(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

	{
		uint8_t* stagingMemory = reinterpret_cast<uint8_t*>(stagingBuffer->map());
		size_t offset = 0;

		for (auto& staged : stagedObjects) {
			memcpy(stagingMemory + offset, staged.inputMemory, staged.inputSize);

			if (staged.deletor) {
				staged.deletor();
			}

			switch (staged.type) {
				case STAGING_TYPE_BUFFER:
				{
					VkBufferCopy copy{};
					copy.srcOffset = offset;
					copy.dstOffset = 0;
					copy.size = staged.inputSize;

					vkCmdCopyBuffer(cmd, stagingBuffer->get_buffer(), staged.buffer.buffer,
							1, &copy);
				}
					break;
				case STAGING_TYPE_IMAGE:
				{
					VkImageMemoryBarrier barrierToTransfer{};
					barrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					barrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrierToTransfer.image = staged.image.image;
					barrierToTransfer.subresourceRange = staged.image.range;
					barrierToTransfer.srcAccessMask = 0;
					barrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrierToTransfer.srcQueueFamilyIndex = context->get_transfer_queue_family();
					barrierToTransfer.dstQueueFamilyIndex = context->get_transfer_queue_family();

					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
							VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
							1, &barrierToTransfer);

					VkBufferImageCopy copy{};
					copy.bufferOffset = offset;
					copy.bufferRowLength = 0;
					copy.bufferImageHeight = 0;
					copy.imageSubresource.aspectMask = staged.image.range.aspectMask;
					copy.imageSubresource.mipLevel = staged.image.range.baseMipLevel;
					copy.imageSubresource.baseArrayLayer = staged.image.range.baseArrayLayer;
					copy.imageSubresource.layerCount = staged.image.range.layerCount;
					copy.imageExtent = staged.image.extent;

					vkCmdCopyBufferToImage(cmd, stagingBuffer->get_buffer(), staged.image.image,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

					VkImageMemoryBarrier barrierToGraphics = barrierToTransfer;
					barrierToGraphics.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrierToGraphics.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					if (staged.image.range.levelCount > 1) {
						barrierToGraphics.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						barrierToGraphics.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					}
					else {
						barrierToGraphics.newLayout = staged.image.finalLayout;
						barrierToGraphics.dstAccessMask = staged.image.finalAccess;
					}

					barrierToGraphics.dstQueueFamilyIndex = context->get_graphics_queue_family();

					if (staged.image.range.levelCount > 1) {
						vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &barrierToGraphics);
					}
					else {
						vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &barrierToGraphics);

						vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &barrierToGraphics);
					}

					int32_t mipWidth = static_cast<int32_t>(staged.image.extent.width);
					int32_t mipHeight = static_cast<int32_t>(staged.image.extent.height);

					for (uint32_t i = 1; i < staged.image.range.levelCount; ++i) {
						VkImageMemoryBarrier barrierMipTransfer = barrierToTransfer;
						barrierMipTransfer.subresourceRange.baseMipLevel = i - 1;
						barrierMipTransfer.subresourceRange.levelCount = 1;
						barrierMipTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						barrierMipTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						barrierMipTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						barrierMipTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						barrierMipTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						barrierMipTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

						vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &barrierMipTransfer);

						VkImageBlit blit{};
						blit.srcOffsets[0] = {0, 0, 0};
						blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
						blit.srcSubresource.aspectMask = staged.image.range.aspectMask;
						blit.srcSubresource.mipLevel = i - 1;
						blit.srcSubresource.baseArrayLayer = 0;
						blit.srcSubresource.layerCount = 1;
						blit.dstOffsets[0] = {0, 0, 0};
						blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
								mipHeight > 1 ? mipHeight / 2 : 1, 1 };
						blit.dstSubresource.aspectMask = staged.image.range.aspectMask;
						blit.dstSubresource.mipLevel = i;
						blit.dstSubresource.baseArrayLayer = 0;
						blit.dstSubresource.layerCount = 1;

						vkCmdBlitImage(graphicsCmd, staged.image.image,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staged.image.image,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

						VkImageMemoryBarrier barrierToDest = barrierMipTransfer;
						barrierToDest.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						barrierToDest.newLayout = staged.image.finalLayout;
						barrierToDest.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						barrierToDest.dstAccessMask = staged.image.finalAccess;

						vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &barrierToDest);

						if (mipWidth > 1) {
							mipWidth /= 2;
						}

						if (mipHeight > 1) {
							mipHeight /= 2;
						}
					}

					if (staged.image.range.levelCount > 1) {
						VkImageMemoryBarrier finalBarrier = barrierToTransfer;
						finalBarrier.subresourceRange.baseMipLevel
								= staged.image.range.levelCount - 1;
						finalBarrier.subresourceRange.levelCount = 1;
						finalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						finalBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						finalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						finalBarrier.dstAccessMask = staged.image.finalAccess;
						finalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						finalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

						vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
								1, &finalBarrier);
					}
				}
					break;
			}

			offset += staged.inputSize;
		}

		stagingBuffer->unmap();
	}

	VK_CHECK(vkEndCommandBuffer(cmd));
	VK_CHECK(vkEndCommandBuffer(graphicsCmd));

	VkSemaphore sem;
	VkSemaphoreCreateInfo semInfo{};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK(vkCreateSemaphore(context->get_device(), &semInfo, nullptr, &sem));

	VkSubmitInfo transferSubmit = vkinit::submit_info(cmd);
	VkSubmitInfo graphicsSubmit = vkinit::submit_info(graphicsCmd);

	transferSubmit.signalSemaphoreCount = 1;
	transferSubmit.pSignalSemaphores = &sem;

	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	graphicsSubmit.waitSemaphoreCount = 1;
	graphicsSubmit.pWaitSemaphores = &sem;
	graphicsSubmit.pWaitDstStageMask = &flags;

	VK_CHECK(vkQueueSubmit(context->get_transfer_queue(), 1, &transferSubmit, VK_NULL_HANDLE));
	VK_CHECK(vkQueueSubmit(context->get_graphics_queue(), 1, &graphicsSubmit, fence));

	vkWaitForFences(context->get_device(), 1, &fence, true, UINT64_MAX);
	vkResetFences(context->get_device(), 1, &fence);

	vkFreeCommandBuffers(context->get_device(), commandPool, 1, &cmd);
	VK_CHECK(vkResetCommandBuffer(graphicsCmd, 0));

	vkDestroySemaphore(context->get_device(), sem, nullptr);
}

StagingContext::StagingContext(RenderContext& contextIn, VkCommandPool commandPoolIn,
			VkFence fenceIn)
		: context(&contextIn)
		, commandPool(commandPoolIn)
		, fence(fenceIn) {}

