#pragma once

#include <vector>
#include <unordered_map>

#include <core/common.hpp>
#include <core/memory.hpp>

#include <rendering/render_context.hpp>

class ImageDescriptorArray {
	public:
		// FIXME: Support split samplers
		explicit ImageDescriptorArray(uint32_t arraySize);

		NULL_COPY_AND_ASSIGN(ImageDescriptorArray);

		void update();

		void set_sampler(Memory::SharedPtr<Sampler>);

		uint32_t get_image_index(VkImageView);
		VkDescriptorSet get_descriptor_set() const;
	private:
		VkDescriptorSet m_imageDescriptors[RenderContext::FRAMES_IN_FLIGHT];
		std::vector<VkDescriptorImageInfo> m_boundImageInfo;
		std::unordered_map<VkImageView, uint32_t> m_imageIndices;
		size_t m_neededDescriptorUpdates;

		Memory::SharedPtr<Sampler> m_sampler;
};

