#include "instance_bucket.hpp"

#include <cassert>
#include <cstring>

#include <core/logging.hpp>

#include <rendering/render_context.hpp>

InstanceBucket::InstanceBucket(VkBufferUsageFlags usageFlags, size_t instanceSize,
			size_t initialCount)
		: m_buffer(nullptr)
		, m_mapping(nullptr)
		, m_instanceSize(instanceSize)
		, m_usageFlags(usageFlags) {
	m_buffer = g_renderContext->buffer_create(instanceSize * initialCount, m_usageFlags,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
	assert(m_buffer && "Failed to create buffer");
	m_mapping = reinterpret_cast<uint8_t*>(m_buffer->map());
}

void* InstanceBucket::get_or_add_instance(ECS::Entity entity) {
	if (!contains(entity)) {
		insert(entity);
		ensure_capacity();
		auto* res = m_mapping + ((size() - 1) * m_instanceSize);
		memset(res, 0, m_instanceSize);

		return res;
	}
	else {
		return m_mapping + get_sparse_index(entity) * m_instanceSize;
	}
}

void InstanceBucket::remove_instance(ECS::Entity entity) {
	if (contains(entity)) {
		memcpy(m_mapping + get_sparse_index(entity) * m_instanceSize,
				m_mapping + (size() - 1) * m_instanceSize, m_instanceSize);
		remove(entity);
	}
}

uint64_t InstanceBucket::get_type_id() const {
	return 0;
}

VkBuffer InstanceBucket::get_buffer() const {
	return *m_buffer;
}

void InstanceBucket::ensure_capacity() {
	if (size() * m_instanceSize > m_buffer->get_size()) {
		auto newCapacity = 2 * size() * m_instanceSize;
		auto buffer = g_renderContext->buffer_create(newCapacity, m_usageFlags,
				VMA_MEMORY_USAGE_CPU_TO_GPU);
		assert(buffer && "Failed to recreate buffer");
		auto* mapping = reinterpret_cast<uint8_t*>(buffer->map());

		memcpy(mapping, m_mapping, m_buffer->get_size());

		m_buffer = std::move(buffer);
		m_mapping = mapping;
	}
}

void InstanceBucket::debug_print() {
	for (auto e : get_dense()) {
		LOG_TEMP("Overstaying its welcome: %d", e);
	}
}

