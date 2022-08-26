#pragma once

#include <vector>
#include <type_traits>

#include <core/events.hpp>

#include <ecs/ecs_fwd.hpp>
#include <ecs/sparse_set.hpp>
#include <ecs/type_id_generator.hpp>

namespace ECS {

template <typename Component, typename = void>
class ComponentPool : public SparseSet {
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

		void remove(Entity entity) override {
			const auto index = get_sparse_index(entity);

			m_removedEvent.fire(entity, m_components[index]);

			std::swap(m_components[index], m_components.back());
			m_components.pop_back();

			SparseSet::remove(entity);
		}

		void clear() override {
			if (!m_removedEvent.empty()) {
				auto& dense = get_dense();

				for (size_t i = 0, l = dense.size(); i < l; ++i) {
					m_removedEvent.fire(dense[i], m_components[i]);
				}
			}

			SparseSet::clear();
			m_components.clear();
		}

		uint64_t get_type_id() const override {
			return TypeIDGenerator::get_type_id<Component>();
		}

		Component* get_components() {
			return m_components.data();
		}

		size_t get_component_count() const {
			return m_components.size();
		}

		size_t capacity() {
			return m_components.capacity();
		}

		Component& back() {
			return m_components.back();
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
		std::vector<Component> m_components;
		ComponentEvent m_addedEvent;
		ComponentEvent m_removedEvent;
};

template <typename Component>
class ComponentPool<Component, std::enable_if_t<std::is_empty_v<Component>>> : public SparseSet {
	public:
		using ComponentEvent = Event::Dispatcher<Entity>;

		// FIXME: error on call
		Component& get(Entity) {
			return m_value;
		}

		// FIXME: error on call
		Component& get_by_index(size_t) {
			return m_value;
		}

		template <typename... Args>
		void emplace(Entity entity, Args&&...) {
			insert(entity);
			m_addedEvent.fire(entity);
		}

		void remove(Entity entity) override {
			m_removedEvent.fire(entity);
			SparseSet::remove(entity);
		}

		void clear() override {
			if (!m_removedEvent.empty()) {
				for (auto entity : get_dense()) {
					m_removedEvent.fire(entity);
				}
			}

			SparseSet::clear();
		}

		void find_and_swap_to(Entity entity, size_t destIndex) {
			const auto srcIndex = get_sparse_index(entity);
			swap(srcIndex, destIndex);
		}

		uint64_t get_type_id() const override {
			return TypeIDGenerator::get_type_id<Component>();
		}

		Component& back() {
			return m_value;
		}

		ComponentEvent& added_event() {
			return m_addedEvent;
		}

		ComponentEvent& removed_event() {
			return m_removedEvent;
		}
	private:
		ComponentEvent m_addedEvent;
		ComponentEvent m_removedEvent;
		Component m_value;
};

}

