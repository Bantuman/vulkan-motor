#include "image.hpp"

#include <cassert>

#include <rendering/vk_common.hpp>
#include <rendering/render_context.hpp>

Buffer::Buffer(RenderContext& contextIn, VkBuffer bufferIn, VmaAllocation allocationIn,
			VkDeviceSize sizeIn)
		: buffer(bufferIn)
		, allocation(allocationIn)
		, size(sizeIn)
		, context(&contextIn) 
		, mapped(false) {}

Buffer::~Buffer() {
	if (buffer != VK_NULL_HANDLE) {
		if (mapped) {
			unmap();
		}

		context->queue_delete([context=this->context,
				buffer=this->buffer, allocation=this->allocation] {
			vmaDestroyBuffer(context->get_allocator(), buffer, allocation);
		});
	}
}

Buffer::Buffer(Buffer&& other)
		: buffer(other.buffer)
		, allocation(other.allocation)
		, size(other.size)
		, context(other.context) {
	other.buffer = VK_NULL_HANDLE;
}

Buffer& Buffer::operator=(Buffer&& other) {
	buffer = other.buffer;
	allocation = other.allocation;
	size = other.size;
	context = other.context;

	other.buffer = VK_NULL_HANDLE;

	return *this;
}

Buffer::operator VkBuffer() const {
	return buffer;
}

VkBuffer Buffer::get_buffer() const {
	return buffer;
}

VmaAllocation Buffer::get_allocation() const {
	return allocation;
}

VkDeviceSize Buffer::get_size() const {
	return size;
}

void* Buffer::map() {
	assert(!mapped);

	void* mapping{};
	VK_CHECK(vmaMapMemory(context->get_allocator(), allocation, &mapping));
	mapped = true;

	return mapping;
}

void Buffer::unmap() {
	assert(mapped);
	vmaUnmapMemory(context->get_allocator(), allocation);
	mapped = false;
}

void Buffer::flush() {
	vmaFlushAllocation(context->get_allocator(), allocation, 0, size);
}

void Buffer::invalidate() {
	vmaInvalidateAllocation(context->get_allocator(), allocation, 0, size);
}

