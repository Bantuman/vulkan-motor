#pragma once

#include <volk.h>

namespace vkinit {
	VkImageCreateInfo image_create_info(VkFormat, VkImageUsageFlags, VkExtent3D);
	VkImageViewCreateInfo image_view_create_info(VkImageViewType, VkFormat, VkImage,
			VkImageAspectFlags, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

	VkSamplerCreateInfo sampler_create_info(VkFilter filters,
			VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			uint32_t mipLevels = 1);

	VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType,
			VkShaderStageFlags, uint32_t binding);

	VkDescriptorBufferInfo descriptor_buffer_info(VkBuffer, VkDeviceSize offset,
			VkDeviceSize range);
	VkDescriptorImageInfo descriptor_image_info(VkImageView, VkSampler sampler = VK_NULL_HANDLE,
			VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType, VkDescriptorSet,
			const VkDescriptorBufferInfo*, uint32_t binding);
	VkWriteDescriptorSet write_descriptor_image(VkDescriptorType, VkDescriptorSet,
			const VkDescriptorImageInfo*, uint32_t binding);

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
	VkSubmitInfo submit_info(VkCommandBuffer& cmd);

	VkImageMemoryBarrier image_barrier(VkImage image, VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout,
			VkImageAspectFlags aspectMask);
}

