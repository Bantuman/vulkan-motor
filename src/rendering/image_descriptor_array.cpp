#include "image_descriptor_array.hpp"

#include <cassert>

ImageDescriptorArray::ImageDescriptorArray(uint32_t arraySize)
		: m_sampler(nullptr) {
	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		m_imageDescriptors[i] = g_renderContext->global_descriptor_set_begin()
			.bind_dynamic_array(0, arraySize, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}
}

void ImageDescriptorArray::update() {
	if (m_neededDescriptorUpdates > 0) {
		--m_neededDescriptorUpdates;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = static_cast<uint32_t>(m_boundImageInfo.size());
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = m_boundImageInfo.data();
		write.dstBinding = 0;
		write.dstSet = get_descriptor_set();

		vkUpdateDescriptorSets(g_renderContext->get_device(), 1, &write, 0, nullptr);
	}
}

void ImageDescriptorArray::set_sampler(Memory::SharedPtr<Sampler> sampler) {
	m_sampler = std::move(sampler);
}

uint32_t ImageDescriptorArray::get_image_index(VkImageView imageView) {
	if (auto it = m_imageIndices.find(imageView); it != m_imageIndices.end()) {
		return it->second;
	}

	assert(m_sampler && "Must have defined a sampler");

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageView = imageView;
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgInfo.sampler = *m_sampler;

	auto newIndex = static_cast<uint32_t>(m_boundImageInfo.size());

	m_boundImageInfo.emplace_back(std::move(imgInfo));
	m_imageIndices.emplace(std::make_pair(imageView, newIndex));

	m_neededDescriptorUpdates = RenderContext::FRAMES_IN_FLIGHT;
	return newIndex;
}

VkDescriptorSet ImageDescriptorArray::get_descriptor_set() const {
	return m_imageDescriptors[g_renderContext->get_frame_index()];
}

