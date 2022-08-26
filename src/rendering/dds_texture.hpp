#pragma once

#include <core/common.hpp>
#include <volk.h>

namespace DDS {

struct DDSTextureInfo {
	uint32_t width;
	uint32_t height;
	uint32_t size;
	VkFormat format;
	uint32_t mipMapCount;
	const uint8_t* dataStart;
	bool isCubeMap;
	bool hasMipMaps;
	bool isCompressed;
	bool hasAlpha;
};

bool get_texture_info(DDSTextureInfo& textureInfo, const void* memory, size_t size,
		bool srgb);

}

