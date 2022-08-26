#pragma once

#include <algorithm>
#include <tuple>

#include <ecs/component_pool.hpp>

namespace ECS {

template <typename... Components>
class View {
	public:
		explicit View(ComponentPool<Components>&... pools)
				: m_smallestPool(std::min<const SparseSet*>({&pools...},
					[](const auto* a, const auto* b) { return a->size() < b->size(); }))
				, m_pools(&pools...) {}

		template <typename Functor>
		void for_each(Functor&& func) {
			for (auto entity : m_smallestPool->get_dense()) {
				if (has_all_components(entity)) {
					func(entity,
							std::get<ComponentPool<Components>*>(m_pools)->get(entity)...);
				}
			}
		}

		template <typename Functor>
		void for_each_cond(Functor&& func) {
			for (auto entity : m_smallestPool->get_dense()) {
				if (has_all_components(entity)) {
					if (auto res = func(entity,
							std::get<ComponentPool<Components>*>(m_pools)->get(entity)...);
							res == IterationDecision::BREAK) {
						return;
					}
				}
			}
		}
	private:
		const SparseSet* m_smallestPool;
		std::tuple<ComponentPool<Components>*...> m_pools;

		bool has_all_components(Entity entity) {
			return (std::get<ComponentPool<Components>*>(m_pools)->contains(entity) && ...);
		}
};

template <typename Component>
class View<Component> {
	public:
		explicit View(ComponentPool<Component>& pool)
				: m_pool(&pool) {}

		template <typename Functor>
		void for_each(Functor&& func) {
			auto* components = m_pool->get_components();
			auto componentCount = m_pool->get_component_count();
			auto& entities = m_pool->get_dense();

			for (size_t i = 0; i < componentCount; ++i) {
				func(entities[i], components[i]);
			}
		}

		template <typename Functor>
		void for_each_cond(Functor&& func) {
			auto* components = m_pool->get_components();
			auto componentCount = m_pool->get_component_count();
			auto& entities = m_pool->get_dense();

			for (size_t i = 0; i < componentCount; ++i) {
				if (auto res = func(entities[i], components[i]);
						res == IterationDecision::BREAK) {
					return;
				}
			}
		}
	private:
		ComponentPool<Component>* m_pool;
};

}

