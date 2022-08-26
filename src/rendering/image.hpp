#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

class RenderContext;

class Image final {
	public:
		explicit Image(VkImage, VmaAllocation, VkExtent3D, VkFormat, VkSampleCountFlagBits);
		~Image();

		Image(Image&&);
		Image& operator=(Image&&);

		Image(const Image&) = delete;
		void operator=(const Image&) = delete;

		void delete_late();

		operator VkImage() const;

		VkImage get_image() const;
		VmaAllocation get_allocation() const;
		VkExtent3D get_extent() const;
		VkFormat get_format() const;
		VkSampleCountFlagBits get_sample_count() const;
	private:
		VkImage m_image;
		VmaAllocation m_allocation;
		VkExtent3D m_extent;
		VkFormat m_format;
		VkSampleCountFlagBits m_sampleCount;
};

