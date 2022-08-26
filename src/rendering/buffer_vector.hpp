#pragma once

#include <cassert>

#include <core/memory.hpp>

#include <rendering/buffer.hpp>
#include <rendering/render_context.hpp>

template <typename T, VkBufferUsageFlags UsageFlags,
		 size_t InitialCapacity = 256,
		 VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
		 VkMemoryPropertyFlags RequiredFlags = 0>
class BufferVector {
	public:
		static_assert(InitialCapacity > 0, "Initial capacity must be > 0");

		using value_type = T;
		using size_type = size_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = T*;
		using iterator = T*;
		using const_iterator = const T*;

		explicit BufferVector()
				: m_buffer(g_renderContext->buffer_create(InitialCapacity * sizeof(T), UsageFlags,
							MemoryUsage, RequiredFlags))
				, m_bufferBegin(reinterpret_cast<T*>(m_buffer->map()))
				, m_bufferEnd(m_bufferBegin) {}

		BufferVector(BufferVector&& other) noexcept
				: m_buffer(std::move(other.m_buffer))
				, m_bufferBegin(other.m_bufferBegin)
				, m_bufferEnd(other.m_bufferEnd) {
			other.m_buffer = nullptr;
			other.m_bufferBegin = nullptr;
			other.m_bufferEnd = nullptr;
		}

		BufferVector& operator=(BufferVector&& other) noexcept {
			m_buffer = std::move(other.m_buffer);
			m_bufferBegin = other.m_bufferBegin;
			m_bufferEnd = other.m_bufferEnd;

			other.m_buffer = nullptr;
			other.m_bufferBegin = nullptr;
			other.m_bufferEnd = nullptr;

			return *this;
		}

		BufferVector(const BufferVector&) = delete;
		void operator=(const BufferVector&) = delete;

		~BufferVector() {
			Memory::destroy(m_bufferBegin, m_bufferEnd);
		}

		iterator begin() {
			return m_bufferBegin;
		}

		iterator end() {
			return m_bufferEnd;
		}

		const_iterator cbegin() const {
			return m_bufferBegin;
		}

		const_iterator cend() const {
			return m_bufferEnd;
		}

		void push_back(const T& value) {
			ensure_capacity();
			Memory::construct_at(m_bufferEnd, value);
			++m_bufferEnd;
		}

		void push_back(T&& value) {
			ensure_capacity();
			Memory::construct_at(m_bufferEnd, value);
			++m_bufferEnd;
		}

		template <typename... Args>
		void emplace_back(Args&&... args) {
			ensure_capacity();
			Memory::construct_at(m_bufferEnd, std::forward<Args>(args)...);
			++m_bufferEnd;
		}

		void pop_back() {
			assert(!empty() && "Cannot pop_back() an empty vector");
			Memory::destroy_at(m_bufferEnd - 1);
			--m_bufferEnd;
		}

		void reserve(size_type cap) {
			reserve_bytes(cap * sizeof(T));
		}

		void clear() {
			Memory::destroy(m_bufferBegin, m_bufferEnd);
			m_bufferEnd = m_bufferBegin;
		}

		reference operator[](size_type index) {
			return m_bufferBegin[index];
		}

		const_reference operator[](size_type index) const {
			return m_bufferBegin[index];
		}

		reference front() {
			return m_bufferBegin[0];
		}

		const_reference front() const {
			return m_bufferBegin[0];
		}

		reference back() {
			return m_bufferEnd[-1];
		}

		const_reference back() const {
			return m_bufferEnd[-1];
		}

		bool empty() const {
			return m_bufferBegin == m_bufferEnd;
		}

		size_type size() const {
			return m_bufferEnd - m_bufferBegin;
		}

		size_type capacity() const {
			return m_buffer->get_size() / sizeof(T);
		}

		size_type byte_capacity() const {
			return m_buffer->get_size();
		}

		T* data() {
			return m_bufferBegin;
		}

		const T* data() const {
			return m_bufferBegin;
		}

		VkBuffer buffer() const {
			return m_buffer->get_buffer();
		}
	private:
		Memory::SharedPtr<Buffer> m_buffer;
		T* m_bufferBegin;
		T* m_bufferEnd;

		void ensure_capacity() {
			if (sizeof(T) * size() == byte_capacity()) {
				reserve_bytes(2 * byte_capacity());
			}
		}

		void reserve_bytes(size_t numBytes) {
			auto newBuffer = g_renderContext->buffer_create(numBytes, UsageFlags, MemoryUsage,
					RequiredFlags);
			T* newBegin = reinterpret_cast<T*>(newBuffer->map());
			T* newEnd = newBegin + size();

			Memory::uninitialized_move(m_bufferBegin, m_bufferEnd, newBegin);
			Memory::destroy(m_bufferBegin, m_bufferEnd);

			m_buffer = newBuffer;
			m_bufferBegin = newBegin;
			m_bufferEnd = newEnd;
		}
};

