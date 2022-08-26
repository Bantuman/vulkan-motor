#include "ecs.hpp"

using namespace ECS;

static constexpr EntityGeneration_T get_generation(Entity entity) {
	return static_cast<EntityGeneration_T>((entity >> GENERATION_SHIFT) & GENERATION_MASK);
}

static constexpr Entity make_entity(EntityIndex_T index, EntityGeneration_T generation) {
	return static_cast<Entity>((index & INDEX_MASK) | (generation << GENERATION_SHIFT));
}

static constexpr void increment_generation(Entity& entity) {
	EntityGeneration_T generation = get_generation(entity) + 1;
	generation += generation == get_generation(INVALID_ENTITY);

	entity = make_entity(get_index(entity), generation);
}

static constexpr void swap_indices(Entity& lhs, Entity& rhs) {
	EntityIndex_T temp = get_index(lhs);
	lhs = make_entity(get_index(rhs), get_generation(lhs));
	rhs = make_entity(temp, get_generation(rhs));
}

// SparseSet

void SparseSet::insert(Entity entity) {
	if (m_sparse.size() <= get_index(entity)) {
		m_sparse.resize(get_index(entity) + 1, INVALID_ENTITY);
	}

	m_sparse[get_index(entity)] = static_cast<Entity>(m_dense.size());
	m_dense.push_back(entity);
}

void SparseSet::remove(Entity entity) {
	if (contains(entity)) {
		const Entity last = m_dense.back();

		m_dense[m_sparse[get_index(entity)]] = last;
		m_sparse[get_index(last)] = m_sparse[get_index(entity)];
		m_sparse[get_index(entity)] = INVALID_ENTITY;

		m_dense.pop_back();
	}
}

void SparseSet::clear() {
	// FIXME: this might be faster if I had pages
	for (auto entity : m_dense) {
		m_sparse[get_index(entity)] = INVALID_ENTITY;
	}

	m_dense.clear();
}

bool SparseSet::contains(Entity entity) const {
	return m_sparse.size() > get_index(entity) && m_sparse[get_index(entity)] != INVALID_ENTITY;
}

bool SparseSet::contains_before_index(Entity entity, size_t indexEnd) const {
	return contains(entity) && m_sparse[get_index(entity)] < indexEnd;
}

bool SparseSet::empty() const {
	return m_dense.empty();
}

size_t SparseSet::size() const {
	return m_dense.size();
}

void SparseSet::swap_to_match(const SparseSet& other, size_t numToSwap) {
	numToSwap = size() < numToSwap ? size() : numToSwap;

	for (size_t i = 0; i < numToSwap; ++i) {
		if (m_dense[i] != other.m_dense[i]) {
			const auto swapIndex = get_index(m_sparse[get_index(other.m_dense[i])]);
			swap(i, swapIndex);
		}
	}
}

void SparseSet::swap(size_t i, size_t j) {
	std::swap(m_sparse[get_index(m_dense[i])], m_sparse[get_index(m_dense[j])]);
	std::swap(m_dense[i], m_dense[j]);
}

std::vector<Entity>& SparseSet::get_dense() {
	return m_dense;
}

const std::vector<Entity>& SparseSet::get_dense() const {
	return m_dense;
}

size_t SparseSet::get_sparse_index(Entity entity) const {
	return m_sparse[get_index(entity)];
}

// Manager
BaseGroup::~BaseGroup() {};

Manager::~Manager() {
	for_each_entity([this](auto entity) {
		destroy_entity(entity);
	});
}

Entity Manager::create_entity() {
	if (m_freeList == INVALID_ENTITY) {
		m_entities.push_back(m_entities.size());
		return m_entities.back();
	}
	else {
		const Entity recycledEntity = make_entity(get_index(m_freeList),
				get_generation(m_entities[get_index(m_freeList)]));
		swap_indices(m_entities[get_index(m_freeList)], m_freeList);
		return recycledEntity;
	}
}

void Manager::destroy_entity(Entity entity) {
	for (auto& [_, pPool] : m_componentPools) {
		if (pPool->contains(entity)) {
			for (auto it = m_groups.rbegin(), end = m_groups.rend(); it != end; ++it) {
				if ((*it)->contains_type(pPool->get_type_id())) {
					(*it)->on_entity_removed(entity);
				}
			}

			pPool->remove(entity);
		}
	}

	increment_generation(m_entities[get_index(entity)]);
	swap_indices(m_freeList, m_entities[get_index(entity)]);
}

void Manager::insert_new_group_sorted(BaseGroup* pGroup) {
	if (m_groups.empty()) {
		m_groups.emplace_back(pGroup);
		return;
	}

	for (auto it = m_groups.begin(), end = m_groups.end(); it != end; ++it) {
		if ((*it)->get_num_types() > pGroup->get_num_types()) {
			m_groups.emplace(it, pGroup);
			return;
		}
	}

	m_groups.emplace_back(pGroup);
}

#include <core/logging.hpp>

void Manager::dump() {
	for (auto& [typeID, pSet] : m_componentPools) {
		LOG_TEMP("TYPE %u", typeID);

		for (auto e : pSet->get_dense()) {
			LOG_TEMP("%d", e);
		}
	}
}

