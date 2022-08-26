#pragma once

#include <ecs/component_pool.hpp>

#include <rendering/buffer_vector.hpp>

#include <core/logging.hpp>

namespace ECS {

struct GPUBufferObject {};

template <typename T>
struct GPUBufferObjectTraits {
	static constexpr VkBufferUsageFlags USAGE_FLAGS = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	static constexpr size_t INITIAL_CAPACITY = 256;
	static constexpr VmaMemoryUsage MEMORY_USAGE = VMA_MEMORY_USAGE_CPU_ONLY;
	static constexpr VkMemoryPropertyFlags REQUIRED_FLAGS = 0;
};

template <typename Component, VkBufferUsageFlags UsageFlags, size_t InitialCapacity,
		 VmaMemoryUsage MemoryUsage, VkMemoryPropertyFlags RequiredFlags>
class BufferComponentPool : public SparseSet {
	public:
		using ComponentEvent = Event::Dispatcher<Entity, Component&>;

		Component& get(Entity key) {
			return m_components[get_sparse_index(key)];
		}

		Component& get_by_index(size_t index) {
			return m_components[index];
		}

		template <typename... Args>
		void emplace(Entity entity, Args&&... args) {
			insert(entity);

			if constexpr (std::is_aggregate_v<Component>) {
				m_components.emplace_back(Component{std::forward<Args>(args)...});
			}
			else {
				m_components.emplace_back(std::forward<Args>(args)...);
			}

			m_addedEvent.fire(entity, m_components.back());
		}

		void find_and_swap_to(Entity entity, size_t destIndex) {
			const auto srcIndex = get_sparse_index(entity);
			swap(srcIndex, destIndex);
		}

		virtual void remove(Entity entity) override {
			const auto index = get_sparse_index(entity);

			m_removedEvent.fire(entity, m_components[index]);

			std::swap(m_components[index], m_components.back());
			m_components.pop_back();

			SparseSet::remove(entity);
		}

		virtual void clear() override {
			if (!m_removedEvent.empty()) {
				auto& dense = get_dense();

				for (size_t i = 0, l = dense.size(); i < l; ++i) {
					m_removedEvent.fire(dense[i], m_components[i]);
				}
			}

			SparseSet::clear();
			m_components.clear();
		}

		virtual uint64_t get_type_id() const override {
			return TypeIDGenerator::get_type_id<Component>();
		}

		Component* get_components() {
			return m_components.data();
		}

		size_t get_component_count() const {
			return m_components.size();
		}

		Component& back() {
			return m_components.back();
		}

		VkBuffer get_buffer() const {
			return m_components.buffer();
		}

		size_t capacity() const {
			return m_components.capacity();
		}

		ComponentEvent& added_event() {
			return m_addedEvent;
		}

		ComponentEvent& removed_event() {
			return m_removedEvent;
		}

		void swap(size_t i, size_t j) override {
			SparseSet::swap(i, j);
			std::swap(m_components[i], m_components[j]);
		}
	private:
		BufferVector<Component, UsageFlags, InitialCapacity, MemoryUsage, RequiredFlags> m_components;
		ComponentEvent m_addedEvent;
		ComponentEvent m_removedEvent;
};

template <typename Component>
class ComponentPool<Component, std::enable_if_t<std::is_base_of_v<GPUBufferObject, Component>>>
		: public BufferComponentPool<Component, 
		GPUBufferObjectTraits<Component>::USAGE_FLAGS,
		GPUBufferObjectTraits<Component>::INITIAL_CAPACITY,
		GPUBufferObjectTraits<Component>::MEMORY_USAGE,
		GPUBufferObjectTraits<Component>::REQUIRED_FLAGS> {};

}

