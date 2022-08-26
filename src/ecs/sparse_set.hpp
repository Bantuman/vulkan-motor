#pragma once

#include <vector>

#include <ecs/ecs_fwd.hpp>

namespace ECS {

class SparseSet {
	public:
		virtual ~SparseSet() = default;

		void insert(Entity);
		virtual void remove(Entity);
		virtual void clear();

		bool contains(Entity) const;
		bool contains_before_index(Entity, size_t indexEnd) const;

		bool empty() const;
		size_t size() const;

		template <typename Condition>
		size_t swap_back_all(Condition&& cond) {
			for (size_t i = 0; i < m_dense.size(); ++i) {
				if (!cond(m_dense[i])) {
					bool noneLeft = true;

					for (size_t j = i + 1; j < m_dense.size(); ++j) {
						if (cond(m_dense[j])) {
							swap(i, j);
							noneLeft = false;
							break;
						}
					}

					if (noneLeft) {
						return i;
					}
				}
			}

			return m_dense.size();
		}

		void swap_to_match(const SparseSet&, size_t numToSwap);

		virtual void swap(size_t i, size_t j);

		std::vector<Entity>& get_dense();
		const std::vector<Entity>& get_dense() const;

		virtual uint64_t get_type_id() const = 0;

		size_t get_sparse_index(Entity) const;
	private:
		std::vector<Entity> m_sparse;
		std::vector<Entity> m_dense;
};

}

