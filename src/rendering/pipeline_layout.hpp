#pragma once

#include <volk.h>

#include <unordered_map>

class PipelineLayoutCache {
	public:
		explicit PipelineLayoutCache(VkDevice);
		~PipelineLayoutCache();

		struct LayoutInfo {
			VkDescriptorSetLayout setLayouts[4];
			VkPushConstantRange constantRanges[2];
			uint32_t numSetLayouts;
			uint32_t numRanges;

			bool operator==(const LayoutInfo&) const;
			size_t hash() const;
		};

		VkPipelineLayout get(const VkPipelineLayoutCreateInfo&);
	private:
		struct LayoutInfoHash {
			size_t operator()(const LayoutInfo& info) const {
				return info.hash();
			}
		};

		std::unordered_map<LayoutInfo, VkPipelineLayout, LayoutInfoHash> m_layouts;
		VkDevice m_device;
};

