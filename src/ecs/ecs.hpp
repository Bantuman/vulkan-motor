#pragma once

#include <cassert>

#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <core/common.hpp>
#include <core/local.hpp>

#include <ecs/ecs_fwd.hpp>
#include <ecs/component_pool.hpp>
#include <ecs/view.hpp>

namespace ECS {
	class Manager;

	class BaseGroup {
		public:
			virtual ~BaseGroup() = 0;
			virtual void on_entity_added(Entity) = 0;
			virtual void on_entity_removed(Entity) = 0;
			virtual void on_pool_cleared() = 0;

			virtual size_t get_num_types() const = 0;
			virtual bool contains_type(uint64_t typeID) const = 0;

			template <typename Component>
			bool contains_type() const {
				return contains_type(TypeIDGenerator::get_type_id<Component>());
			}

			template <typename... Components>
			bool matches_exactly() const {
				return get_num_types() == sizeof...(Components)
						&& (contains_type<Components>() && ...);
			}
		private:
	};

	template <typename... Components>
	class Group : public BaseGroup {
		public:
			~Group() override = default;
			static_assert(sizeof...(Components) > 1, "Must be >1 components");

			explicit Group(Manager& reg, ComponentPool<Components>&... pools);

			template <typename Functor>
			void for_each(Functor&& func) {
				auto& entities = std::get<0>(m_pools)->get_dense();

				for (size_t i = 0; i < m_numComponents; ++i) {
					func(entities[i],
							std::get<ComponentPool<Components>*>(m_pools)->get_by_index(i)...);
				}
			}

			template <typename Functor>
			void for_each_cond(Functor&& func) {
				auto& entities = std::get<0>(m_pools)->get_dense();

				for (size_t i = 0; i < m_numComponents; ++i) {
					if (auto res = func(entities[i],
							std::get<ComponentPool<Components>*>(m_pools)->get_by_index(i)...);
							res == IterationDecision::BREAK) {
						return;
					}
				}
			}

			template <typename CompareComponent>
			void sort_on_component() {
				for (size_t i = 0; i < m_numComponents; ++i) {
					size_t j = i;

					while (j > 0 && std::get<ComponentPool<CompareComponent>*>(m_pools)
							->get_by_index(j) < std::get<ComponentPool<CompareComponent>*>(m_pools)
							->get_by_index(j - 1)) {
						swap(j, j - 1);
						--j;
					}
				}
			}

			void swap(size_t i, size_t j) {
				(std::get<ComponentPool<Components>*>(m_pools)->swap(i, j), ...);
			}

			void on_entity_added(Entity entity) override {
				if (should_add_entity(entity)) {
					(std::get<ComponentPool<Components>*>(m_pools)->find_and_swap_to(entity,
							m_numComponents), ...);
					++m_numComponents;
				}
			}

			void on_entity_removed(Entity entity) override {
				if (should_remove_entity(entity)) {
					(std::get<ComponentPool<Components>*>(m_pools)->find_and_swap_to(entity,
							m_numComponents - 1), ...);
					--m_numComponents;
				}
			}

			void on_pool_cleared() override {
				m_numComponents = 0;
			}

			size_t get_num_types() const override {
				return sizeof...(Components);
			}

			bool contains_type(uint64_t typeID) const override {
				return m_typeIDs.find(typeID) != m_typeIDs.end();
			}
		private:
			std::tuple<ComponentPool<Components>*...> m_pools;
			size_t m_numComponents;
			std::unordered_set<uint64_t> m_typeIDs;

			template <typename Component>
			void match_smallest_pool(ComponentPool<Component>& pool, SparseSet& smallestPool) {
				if (&pool != &smallestPool) {
					pool.swap_to_match(smallestPool, m_numComponents);
				}
			}

			// whether all pools contains(entity) && dense index of entity not within controlled
			// zone
			bool should_add_entity(Entity entity) const {
				return (std::get<ComponentPool<Components>*>(m_pools)->contains(entity) && ...)
						&& ((std::get<ComponentPool<Components>*>(m_pools)
						->get_sparse_index(entity) >= m_numComponents) && ...);
			}

			// whether any pool (contains(entity) && indexof(entity) < m_numComponents)
			bool should_remove_entity(Entity entity) {
				return (std::get<ComponentPool<Components>*>(m_pools)->contains(entity) && ...);
			}
	};

	class Manager {
		public:
			explicit Manager() = default;
			~Manager();

			Manager(Manager&&) = default;
			Manager& operator=(Manager&&) = default;

			Manager(const Manager&) = delete;
			void operator=(const Manager&) = delete;

			Entity create_entity();
			void destroy_entity(Entity);

			template <typename Component, typename... Args>
			decltype(auto) add_component(Entity entity, Args&&... args) {
				assert(!has_component<Component>(entity));

				auto& pool = get_or_create_pool<Component>();
				pool.emplace(entity, std::forward<Args>(args)...);

				for (auto& pGroup : m_groups) {
					if (pGroup->contains_type<Component>()) {
						pGroup->on_entity_added(entity);
					}
				}

				return pool.back();
			}

			template <typename Component, typename... Args>
			Component& get_or_add_component(Entity entity, Args&&... args) {
				auto& pool = get_or_create_pool<Component>();

				if (pool.contains(entity)) {
					return pool.get(entity);
				}

				return add_component<Component>(entity, args...);
			}

			template <typename Component>
			void remove_component(Entity entity) {
				assert(has_component<Component>(entity));

				for (auto it = m_groups.rbegin(), end = m_groups.rend(); it != end; ++it) {
					if ((*it)->contains_type<Component>()) {
						(*it)->on_entity_removed(entity);
					}
				}

				auto& pool = get_pool<Component>();
				pool.remove(entity);
			}

			template <typename Component>
			bool has_component(Entity entity) const {
				auto it = m_componentPools.find(TypeIDGenerator::get_type_id<Component>());

				if (it == m_componentPools.end() || !it->second) {
					return false;
				}

				return it->second->contains(entity);
			}

			template <typename Component>
			Component& get_component(Entity entity) {
				assert(entity != ECS::INVALID_ENTITY);
				return get_pool<Component>().get(entity);
			}

			template <typename Component>
			Component* try_get_component(Entity entity) {
				auto it = m_componentPools.find(TypeIDGenerator::get_type_id<Component>());

				if (it == m_componentPools.end() || !it->second) {
					return nullptr;
				}

				if (it->second->contains(entity)) {
					auto* pool = static_cast<ComponentPool<Component>*>(it->second.get());
					return &pool->get(entity);
				}
				else {
					return nullptr;
				}
			}

			bool is_valid_entity(Entity entity) const {
				return entity != ECS::INVALID_ENTITY && m_entities.size() > get_index(entity)
						&& m_entities[get_index(entity)] == entity;
			}

			template <typename Component>
			void clear_pool() {
				auto it = m_componentPools.find(TypeIDGenerator::get_type_id<Component>());

				if (it == m_componentPools.end() || !it->second) {
					return;
				}

				auto& pool = *static_cast<ComponentPool<Component>*>(it->second.get());

				for (auto& pGroup : m_groups) {
					if (pGroup->contains_type<Component>()) {
						pGroup->on_pool_cleared();
					}
				}

				pool.clear();
			}

			template <typename... Components>
			Group<Components...>& get_group() {
				if (auto* pGroup = find_group<Components...>(); pGroup) {
					return *pGroup;
				}
				else {
					pGroup = new Group<Components...>(*this, get_or_create_pool<Components>()...);
					insert_new_group_sorted(pGroup);

					return *pGroup;
				}
			}

			template <typename Functor>
			void for_each_entity(Functor&& func) {
				for (size_t i = 0; i < m_entities.size(); ++i) {
					if (get_index(m_entities[i]) == i) {
						func(m_entities[i]);
					}
				}
			}

			template <typename... Components, typename Functor>
			void run_system(Functor&& func) {
				get_view<Components...>().for_each(std::forward<Functor>(func));
			}

			template <typename... Components>
			View<Components...> get_view() {
				return View(get_or_create_pool<Components>()...);
			}

			template <typename Component>
			ComponentPool<Component>& get_pool() {
				return static_cast<ComponentPool<Component>&>(*m_componentPools[
						TypeIDGenerator::get_type_id<Component>()]);
			}

			template <typename Component>
			ComponentPool<Component>& get_or_create_pool() {
				auto& pPool = m_componentPools[TypeIDGenerator::get_type_id<Component>()];

				if (!pPool) {
					pPool.reset(new ComponentPool<Component>{});
				}

				return static_cast<ComponentPool<Component>&>(*pPool);
			}

			template <typename Component>
			const ComponentPool<Component>& get_pool() const {
				return const_cast<Manager*>(this)->get_pool<Component>();
			}

			template <typename Component>
			Component* get_pool_data() {
				return get_pool<Component>().get_components();
			}

			template <typename Component>
			const Component* get_pool_data() const {
				return const_cast<Manager*>(this)->get_pool_data<Component>();
			}

			template <typename Component>
			size_t get_pool_size() const {
				return const_cast<Manager*>(this)->get_pool<Component>().get_component_count();
			}

			template <typename... Components>
			bool has_all_components(Entity entity) const {
				return (get_pool<Components>().contains(entity) && ...);
			}

			template <typename Component>
			decltype(auto) component_added_event() {
				return get_or_create_pool<Component>().added_event();
			}

			template <typename Component>
			decltype(auto) component_removed_event() {
				return get_or_create_pool<Component>().removed_event();
			}

			void dump();
		private:
			std::unordered_map<uint64_t, std::unique_ptr<SparseSet>> m_componentPools;
			std::vector<std::unique_ptr<BaseGroup>> m_groups;
			std::vector<Entity> m_entities;
			Entity m_freeList = INVALID_ENTITY;

			template <typename... Components>
			Group<Components...>* find_group() {
				for (auto& pGroup : m_groups) {
					if (pGroup->matches_exactly<Components...>()) {
						return static_cast<Group<Components...>*>(pGroup.get());
					}
				}

				return nullptr;
			}

			void insert_new_group_sorted(BaseGroup*);
	};
}

template <typename... Components>
inline ECS::Group<Components...>::Group(Manager& reg, ComponentPool<Components>&... pools)
		: m_pools(&pools...)
		, m_numComponents(0)
		, m_typeIDs{TypeIDGenerator::get_type_id<Components>()...} {

	auto* smallestPool = (std::min<SparseSet*>({&pools...},
			[](const auto& a, const auto& b) { return a->size() < b->size(); }));

	m_numComponents = smallestPool->swap_back_all([reg_ = &reg](Entity entity) {
		return reg_->has_all_components<Components...>(entity);
	});

	(match_smallest_pool(pools, *smallestPool), ...);
}

inline Local<ECS::Manager> g_ecs;

