#include "image.hpp"

#include <rendering/render_context.hpp>

Image::Image(VkImage image, VmaAllocation allocation, VkExtent3D extent, VkFormat format,
			VkSampleCountFlagBits sampleCount)
		: m_image(image)
		, m_allocation(allocation)
		, m_extent(extent)
		, m_format(format)
		, m_sampleCount(sampleCount) {}

Image::~Image() {
	if (m_image != VK_NULL_HANDLE) {
		g_renderContext->queue_delete([image=m_image, allocation=m_allocation] {
			vmaDestroyImage(g_renderContext->get_allocator(), image, allocation);
		});
	}
}

Image::Image(Image&& other)
		: m_image(other.m_image)
		, m_allocation(other.m_allocation)
		, m_extent(other.m_extent)
		, m_format(other.m_format)
		, m_sampleCount(other.m_sampleCount) {
	other.m_image = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& other) {
	m_image = other.m_image;
	m_allocation = other.m_allocation;
	m_extent = other.m_extent;
	m_format = other.m_format;
	m_sampleCount = other.m_sampleCount;

	other.m_image = VK_NULL_HANDLE;

	return *this;
}

void Image::delete_late() {
	if (m_image != VK_NULL_HANDLE) {
		g_renderContext->queue_delete_late([image=m_image, allocation=m_allocation] {
			vmaDestroyImage(g_renderContext->get_allocator(), image, allocation);
		});

		m_image = VK_NULL_HANDLE;
	}
}

Image::operator VkImage() const {
	return m_image;
}

VkImage Image::get_image() const {
	return m_image;
}

VmaAllocation Image::get_allocation() const {
	return m_allocation;
}

VkExtent3D Image::get_extent() const {
	return m_extent;
}

VkFormat Image::get_format() const {
	return m_format;
}

VkSampleCountFlagBits Image::get_sample_count() const {
	return m_sampleCount;
}

