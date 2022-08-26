#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

class RenderContext;

class Buffer final {
	public:
		explicit Buffer(RenderContext&, VkBuffer, VmaAllocation, VkDeviceSize);
		~Buffer();

		Buffer(Buffer&&);
		Buffer& operator=(Buffer&&);

		Buffer(const Buffer&) = delete;
		void operator=(const Buffer&) = delete;

		void* map();
		void unmap();

		void flush();
		void invalidate();

		operator VkBuffer() const;

		VkBuffer get_buffer() const;
		VmaAllocation get_allocation() const;
		VkDeviceSize get_size() const;
	private:
		VkBuffer buffer;
		VmaAllocation allocation;
		VkDeviceSize size;
		RenderContext* context;
		bool mapped;
};

