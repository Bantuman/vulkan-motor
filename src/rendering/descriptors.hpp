#pragma once

#include <volk.h>

#include <unordered_map>
#include <vector>

class DescriptorAllocator {
	public:
		explicit DescriptorAllocator(VkDevice);
		~DescriptorAllocator();

		VkDescriptorSet allocate(VkDescriptorSetLayout, uint32_t dynamicArrayCount = 0);

		void reset_pools();

		VkDevice get_device() const;
	private:
		std::vector<VkDescriptorPool> m_pools;
		std::vector<VkDescriptorPool> m_freePools;
		VkDescriptorPool m_currentPool;
		VkDevice m_device;

		VkDescriptorPool create_pool();
};

class DescriptorLayoutCache {
	public:
		explicit DescriptorLayoutCache(VkDevice);
		~DescriptorLayoutCache();

		struct LayoutInfo {
			VkDescriptorSetLayoutBinding bindings[8];
			size_t numBindings;
			bool hasUnboundedArray;

			bool operator==(const LayoutInfo&) const;
			size_t hash() const;
		};

		VkDescriptorSetLayout get(const VkDescriptorSetLayoutCreateInfo&);
	private:
		struct LayoutInfoHash {
			size_t operator()(const LayoutInfo& info) const {
				return info.hash();
			}
		};

		std::unordered_map<LayoutInfo, VkDescriptorSetLayout, LayoutInfoHash> m_setLayouts;
		VkDevice m_device;
};

class DescriptorBuilder {
	public:
		explicit DescriptorBuilder(DescriptorLayoutCache&, DescriptorAllocator&);

		DescriptorBuilder& bind_buffer(uint32_t binding, const VkDescriptorBufferInfo&,
				VkDescriptorType, VkShaderStageFlags);

		DescriptorBuilder& bind_image(uint32_t binding, const VkDescriptorImageInfo&,
				VkDescriptorType, VkShaderStageFlags);
		DescriptorBuilder& bind_images(uint32_t binding, const VkDescriptorImageInfo*,
				uint32_t count, VkDescriptorType, VkShaderStageFlags);

		DescriptorBuilder& bind_dynamic_array(uint32_t binding, uint32_t maxSize,
				VkDescriptorType, VkShaderStageFlags);

		VkDescriptorSet build();
	private:
		// FIXME: make this frame-local memory
		std::vector<VkWriteDescriptorSet> m_writes;
		std::vector<VkDescriptorSetLayoutBinding> m_bindings;
		uint32_t m_dynamicArrayIndex;

		DescriptorLayoutCache& m_layoutCache;
		DescriptorAllocator& m_allocator;
};

