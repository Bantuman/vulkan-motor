#include "vk_initializers.hpp"

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usageFlags,
		VkExtent3D extent) {
	VkImageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = extent;

	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	return info;
}

VkImageViewCreateInfo vkinit::image_view_create_info(VkImageViewType viewType, VkFormat format,
		VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t arrayLayers) {
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	
	info.viewType = viewType;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = mipLevels;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = arrayLayers;
	info.subresourceRange.aspectMask = aspectFlags;

	return info;
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkFilter filters,
		VkSamplerAddressMode samplerAddressMode, uint32_t mipLevels) {
	VkSamplerCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.minFilter = filters;
	info.magFilter = filters;
	info.addressModeU = samplerAddressMode;
	info.addressModeV = samplerAddressMode;
	info.addressModeW = samplerAddressMode;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.minLod = 0.f;
	info.maxLod = static_cast<float>(mipLevels);
	info.mipLodBias = 0.f;

	return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptor_set_layout_binding(VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags, uint32_t binding) {
	VkDescriptorSetLayoutBinding setBind{};
	setBind.binding = binding;
	setBind.descriptorCount = 1;
	setBind.descriptorType = descriptorType;
	setBind.pImmutableSamplers = nullptr;
	setBind.stageFlags = stageFlags;

	return setBind;
}

VkDescriptorBufferInfo vkinit::descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset,
		VkDeviceSize range) {
	return {buffer, offset, range};
}

VkDescriptorImageInfo vkinit::descriptor_image_info(VkImageView imageView, VkSampler sampler,
		VkImageLayout layout) {
	VkDescriptorImageInfo info{};
	info.sampler = sampler;
	info.imageView = imageView;
	info.imageLayout = layout;

	return info;
}

VkWriteDescriptorSet vkinit::write_descriptor_buffer(VkDescriptorType descriptorType,
		VkDescriptorSet descriptorSet, const VkDescriptorBufferInfo* bufferInfo,
		uint32_t binding) {
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = binding;
	write.dstSet = descriptorSet;
	write.descriptorCount = 1;
	write.descriptorType = descriptorType;
	write.pBufferInfo = bufferInfo;

	return write;
}

VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType descriptorType,
		VkDescriptorSet descriptorSet, const VkDescriptorImageInfo* imageInfo, uint32_t binding) {
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = binding;
	write.dstSet = descriptorSet;
	write.descriptorCount = 1;
	write.descriptorType = descriptorType;
	write.pImageInfo = imageInfo;

	return write;
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = flags;

	return info;
}

VkSubmitInfo vkinit::submit_info(VkCommandBuffer& cmd) {
	VkSubmitInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;

	return info;
}

VkImageMemoryBarrier vkinit::image_barrier(VkImage image, VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout,
		VkImageAspectFlags aspectMask) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	return barrier;
}

