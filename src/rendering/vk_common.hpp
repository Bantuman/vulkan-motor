#pragma once

#include <volk.h>
#include <core/logging.hpp>

#define VK_CHECK(x)														\
	do {																\
		if (VkResult err = x; err) {									\
			LOG_ERROR("VULKAN", "%d", err);								\
		}																\
	}																	\
	while (0)

