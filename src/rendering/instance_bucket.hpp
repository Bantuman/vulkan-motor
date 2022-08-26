#pragma once

#include <map>

#include <core/memory.hpp>

#include <ecs/sparse_set.hpp>

#include <rendering/buffer.hpp>

class InstanceBucket : public ECS::SparseSet {
	public:
		explicit InstanceBucket(VkBufferUsageFlags, size_t instanceSize, size_t initialCount);

		void* get_or_add_instance(ECS::Entity);
		void remove_instance(ECS::Entity);

		uint64_t get_type_id() const override;

		VkBuffer get_buffer() const;

		void debug_print();
	private:
		Memory::SharedPtr<Buffer> m_buffer;
		uint8_t* m_mapping;
		size_t m_instanceSize;
		VkBufferUsageFlags m_usageFlags;

		void ensure_capacity();
};

template <typename RenderKey>
class InstanceBucketCollection {
	public:
		using Container = std::map<RenderKey, InstanceBucket>;

		static constexpr size_t INITIAL_COUNT = 512;

		explicit InstanceBucketCollection(VkBufferUsageFlags usageFlags,
					size_t initialCount = INITIAL_COUNT)
				: m_usageFlags(usageFlags)
				, m_initialCount(initialCount) {}

		typename Container::iterator begin() {
			return m_instances.begin();
		}

		typename Container::iterator end() {
			return m_instances.end();
		}

		InstanceBucket& get_bucket(const RenderKey& key) {
			return m_instances.find(key)->second;
		}

		const InstanceBucket& get_bucket(const RenderKey& key) const {
			return m_instances.find(key)->second;
		}

		void* get_or_add_instance(const RenderKey&, size_t instanceSize, ECS::Entity);
		void remove_instance(const RenderKey&, ECS::Entity);
	private:
		Container m_instances;
		VkBufferUsageFlags m_usageFlags;
		size_t m_initialCount;
};

template <typename RenderKey>
inline void* InstanceBucketCollection<RenderKey>::get_or_add_instance(const RenderKey& key,
		size_t instanceSize, ECS::Entity entity) {
	if (auto info = m_instances.find(key); info != m_instances.end()) {
		return info->second.get_or_add_instance(entity);
	}
	else {
		InstanceBucket bucket(m_usageFlags, instanceSize, m_initialCount);
		auto* res = bucket.get_or_add_instance(entity);

		m_instances.emplace(std::make_pair(key, std::move(bucket)));

		return res;
	}
}

template <typename RenderKey>
inline void InstanceBucketCollection<RenderKey>::remove_instance(const RenderKey& key,
		ECS::Entity entity) {
	if (auto info = m_instances.find(key); info != m_instances.end()) {
		info->second.remove_instance(entity);
	}
}

