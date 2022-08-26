#include "descriptors.hpp"

#include <rendering/vk_common.hpp>
#include <pair.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

static constexpr std::array<Pair<VkDescriptorType, float>, 11> g_poolCreateRatios = {
	 VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f,
	 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f,
	 VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f,
	 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f,
	 VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f,
	 VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f,
	 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f,
	 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f,
	 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f,
	 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f,
	 VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f 
};

static constexpr const uint32_t DESCRIPTORS_PER_POOL = 1000;

// DescriptorAllocator

DescriptorAllocator::DescriptorAllocator(VkDevice device)
		 : m_currentPool(VK_NULL_HANDLE)
		 , m_device(device) {}

DescriptorAllocator::~DescriptorAllocator() {
	for (auto pool : m_pools) {
		vkDestroyDescriptorPool(m_device, pool, nullptr);
	}

	for (auto pool : m_freePools) {
		vkDestroyDescriptorPool(m_device, pool, nullptr);
	}
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout,
		uint32_t dynamicArrayCount) {
	if (m_currentPool == VK_NULL_HANDLE) {
		m_currentPool = create_pool();
		m_pools.push_back(m_currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = m_currentPool;
	allocInfo.descriptorSetCount = 1;

	VkDescriptorSetVariableDescriptorCountAllocateInfo setCounts{};

	if (dynamicArrayCount > 0) {
		setCounts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		setCounts.descriptorSetCount = 1;
		setCounts.pDescriptorCounts = &dynamicArrayCount;

		allocInfo.pNext = &setCounts;
	}

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkResult allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet);

	switch (allocResult) {
		case VK_SUCCESS:
			return descriptorSet;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			m_currentPool = create_pool();
			m_pools.push_back(m_currentPool);

			allocInfo.descriptorPool = m_currentPool;

			VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet));

			return descriptorSet;
		default:
			break;
	}

	// FIXME: if the program reaches here, this means there was a fatal error
	return descriptorSet;
}

void DescriptorAllocator::reset_pools() {
	for (auto pool : m_pools) {
		vkResetDescriptorPool(m_device, pool, 0);
		m_freePools.push_back(pool);
	}

	m_pools.clear();
	m_currentPool = VK_NULL_HANDLE;
}

VkDevice DescriptorAllocator::get_device() const {
	return m_device;
}

VkDescriptorPool DescriptorAllocator::create_pool() {
	if (!m_freePools.empty()) {
		auto res = m_freePools.back();
		m_freePools.pop_back();
		return res;
	}

	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(g_poolCreateRatios.size());

	for (auto createRatio : g_poolCreateRatios) {
		sizes.emplace_back(VkDescriptorPoolSize{
			createRatio.first,
			static_cast<uint32_t>(createRatio.second * DESCRIPTORS_PER_POOL)
		});
	}

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.maxSets = DESCRIPTORS_PER_POOL;
	createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
	createInfo.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	VK_CHECK(vkCreateDescriptorPool(m_device, &createInfo, nullptr, &descriptorPool));

	return descriptorPool;
}

// DescriptorLayoutCache

DescriptorLayoutCache::DescriptorLayoutCache(VkDevice device)
		: m_device(device) {}

DescriptorLayoutCache::~DescriptorLayoutCache() {
	for (auto& [_, layout] : m_setLayouts) {
		vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
	}

	m_setLayouts.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::get(
		const VkDescriptorSetLayoutCreateInfo& createInfo) {
	LayoutInfo layoutInfo{};
	layoutInfo.numBindings = createInfo.bindingCount;
	// FIXME: make this check the sType and perhaps the actual structure contents
	layoutInfo.hasUnboundedArray = createInfo.pNext != nullptr;

	assert(createInfo.bindingCount <= std::size(layoutInfo.bindings)
			&& "Must have <= bindings than the layout info storage cap");

	layoutInfo.numBindings = createInfo.bindingCount;
	memcpy(layoutInfo.bindings, createInfo.pBindings,
			createInfo.bindingCount * sizeof(VkDescriptorSetLayoutBinding));

	std::sort(layoutInfo.bindings, layoutInfo.bindings + createInfo.bindingCount,
			[](const auto& a, const auto& b) {
		return a.binding < b.binding;
	});

	if (auto it = m_setLayouts.find(layoutInfo); it != m_setLayouts.end()) {
		return it->second;
	}

	VkDescriptorSetLayout layout;
	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &layout));

	m_setLayouts.emplace(std::make_pair(layoutInfo, layout));
	return layout;
}	

// LayoutInfo

bool DescriptorLayoutCache::LayoutInfo::operator==(const LayoutInfo& other) const {
	if (numBindings != other.numBindings) {
		return false;
	}

	if (hasUnboundedArray != other.hasUnboundedArray) {
		return false;
	}

	for (size_t i = 0; i < numBindings; ++i) {
		if (bindings[i].binding != other.bindings[i].binding) {
			return false;
		}

		if (bindings[i].descriptorType != other.bindings[i].descriptorType) {
			return false;
		}

		if (bindings[i].descriptorCount != other.bindings[i].descriptorCount) {
			return false;
		}

		if (bindings[i].stageFlags != other.bindings[i].stageFlags) {
			return false;
		}
	}

	return true;
}

size_t DescriptorLayoutCache::LayoutInfo::hash() const {
	size_t result = std::hash<size_t>{}(numBindings);

	for (size_t i = 0; i < numBindings; ++i) {
		auto& bnd = bindings[i];
		size_t bindingHash = bnd.binding | (bnd.descriptorType << 8) | (bnd.descriptorCount << 16)
				| (bnd.stageFlags << 24);

		result ^= std::hash<size_t>{}(bindingHash);
	}

	result ^= hasUnboundedArray;

	return result;
}

// DescriptorBuilder

DescriptorBuilder::DescriptorBuilder(DescriptorLayoutCache& layoutCache,
		DescriptorAllocator& allocator)
	: m_dynamicArrayIndex(~0u)
	, m_layoutCache(layoutCache)
	, m_allocator(allocator) {}

DescriptorBuilder& DescriptorBuilder::bind_buffer(uint32_t binding,
		const VkDescriptorBufferInfo& info, VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorCount = 1;
	bindingInfo.descriptorType = descriptorType;
	bindingInfo.stageFlags = stageFlags;
	bindingInfo.binding = binding;

	m_bindings.emplace_back(std::move(bindingInfo));

	VkWriteDescriptorSet writeInfo{};
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.descriptorCount = 1;
	writeInfo.descriptorType = descriptorType;
	writeInfo.pBufferInfo = &info;
	writeInfo.dstBinding = binding;

	m_writes.emplace_back(std::move(writeInfo));

	return *this;
}

DescriptorBuilder& DescriptorBuilder::bind_image(uint32_t binding,
		const VkDescriptorImageInfo& info, VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags) {
	return bind_images(binding, &info, 1, descriptorType, stageFlags);
}

DescriptorBuilder& DescriptorBuilder::bind_images(uint32_t binding,
		const VkDescriptorImageInfo* info, uint32_t count, VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorCount = count;
	bindingInfo.descriptorType = descriptorType;
	bindingInfo.stageFlags = stageFlags;
	bindingInfo.binding = binding;

	m_bindings.emplace_back(std::move(bindingInfo));

	VkWriteDescriptorSet writeInfo{};
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.descriptorCount = count;
	writeInfo.descriptorType = descriptorType;
	writeInfo.pImageInfo = info;
	writeInfo.dstBinding = binding;

	m_writes.emplace_back(std::move(writeInfo));

	return *this;
}

DescriptorBuilder& DescriptorBuilder::bind_dynamic_array(uint32_t binding, uint32_t maxSize,
		VkDescriptorType descriptorType, VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorCount = maxSize;
	bindingInfo.descriptorType = descriptorType;
	bindingInfo.stageFlags = stageFlags;
	bindingInfo.binding = binding;

	m_dynamicArrayIndex = static_cast<uint32_t>(m_bindings.size());
	m_bindings.emplace_back(std::move(bindingInfo));

	return *this;
}

VkDescriptorSet DescriptorBuilder::build() {
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pBindings = m_bindings.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

	std::vector<VkDescriptorBindingFlags> flags;

	uint32_t dynamicCount = 0u;

	if (m_dynamicArrayIndex != ~0u) {
		dynamicCount = m_bindings[m_dynamicArrayIndex].descriptorCount;

		flags.resize(m_bindings.size());
		flags[m_dynamicArrayIndex] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
				| VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
		bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlags.bindingCount = layoutInfo.bindingCount;
		bindingFlags.pBindingFlags = flags.data();

		layoutInfo.pNext = &bindingFlags;
	}

	VkDescriptorSetLayout layout = m_layoutCache.get(layoutInfo);
	VkDescriptorSet descriptorSet = m_allocator.allocate(layout, dynamicCount);

	for (auto& write : m_writes) {
		write.dstSet = descriptorSet;
	}

	vkUpdateDescriptorSets(m_allocator.get_device(), static_cast<uint32_t>(m_writes.size()),
			m_writes.data(), 0, nullptr);

	return descriptorSet;
}

