#include "pipeline_layout.hpp"

#include <rendering/vk_common.hpp>

#include <algorithm>

#include <cassert>
#include <cstring>

PipelineLayoutCache::PipelineLayoutCache(VkDevice device)
		: m_device(device) {}

PipelineLayoutCache::~PipelineLayoutCache() {
	for (auto& [_, layout] : m_layouts) {
		vkDestroyPipelineLayout(m_device, layout, nullptr);
	}
}

VkPipelineLayout PipelineLayoutCache::get(const VkPipelineLayoutCreateInfo& createInfo) {
	assert(createInfo.setLayoutCount <= 4 && "Need to increase amount of stored layouts");
	assert(createInfo.pushConstantRangeCount <= 2 && "Need to increase amount of constant ranges");

	LayoutInfo layoutInfo{};
	layoutInfo.numSetLayouts = createInfo.setLayoutCount;
	layoutInfo.numRanges = createInfo.pushConstantRangeCount;
	memcpy(layoutInfo.setLayouts, createInfo.pSetLayouts,
			createInfo.setLayoutCount * sizeof(VkDescriptorSetLayout));

	if (createInfo.pushConstantRangeCount > 0) {
		memcpy(layoutInfo.constantRanges, createInfo.pPushConstantRanges,
				createInfo.pushConstantRangeCount * sizeof(VkPushConstantRange));
	}

	std::sort(layoutInfo.setLayouts, layoutInfo.setLayouts + layoutInfo.numSetLayouts);

	if (auto it = m_layouts.find(layoutInfo); it != m_layouts.end()) {
		return it->second;
	}

	VkPipelineLayout layout;
	VK_CHECK(vkCreatePipelineLayout(m_device, &createInfo, nullptr, &layout));

	m_layouts.emplace(std::make_pair(layoutInfo, layout));
	return layout;
}

// LayoutInfo

bool PipelineLayoutCache::LayoutInfo::operator==(const LayoutInfo& other) const {
	if (numSetLayouts != other.numSetLayouts) {
		return false;
	}

	if (numRanges != other.numRanges) {
		return false;
	}

	for (uint32_t i = 0; i < numSetLayouts; ++i) {
		if (setLayouts[i] != other.setLayouts[i]) {
			return false;
		}
	}

	for (uint32_t i = 0; i < numRanges; ++i) {
		if (constantRanges[i].stageFlags != other.constantRanges[i].stageFlags) {
			return false;
		}

		if (constantRanges[i].offset != other.constantRanges[i].offset) {
			return false;
		}

		if (constantRanges[i].size != other.constantRanges[i].size) {
			return false;
		}
	}

	return true;
}

size_t PipelineLayoutCache::LayoutInfo::hash() const {
	size_t result = std::hash<uint32_t>{}(numSetLayouts);

	for (uint32_t i = 0; i < numSetLayouts; ++i) {
		result ^= std::hash<size_t>{}(reinterpret_cast<size_t>(setLayouts[i]) << (8 * i));
	}

	for (uint32_t i = 0; i < numRanges; ++i) {
		size_t value = constantRanges[i].stageFlags | (constantRanges[i].size << 8)
				| (constantRanges[i].offset << 16);
		result ^= std::hash<size_t>{}(value << (32 * i));
	}

	return result;
}

