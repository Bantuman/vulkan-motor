#pragma once

#include <rendering/buffer_component_pool.hpp>

#include <math/vector2.hpp>
#include <math/vector4.hpp>
#include <math/transform2d.hpp>

#include <ui/ui_enumerations.hpp>

namespace Game {

struct RectInstance : public ECS::GPUBufferObject {
	Math::Vector4 texLayout;
	Math::Vector4 color;
	Math::Transform2D transform;
	uint32_t imageIndex;
	uint32_t samplerIndex;
};

struct RectOrderInfo {
	uint32_t displayOrder;
	uint32_t screenIndex;
	uint32_t zIndex;
	uint32_t depth;
	uint32_t childIndex;
	Game::ZIndexBehavior zIndexBehavior;
	bool overlay;
};

struct RectPriority {
	uint64_t highOrder;
	uint64_t lowOrder;

	RectPriority() = default;
	RectPriority(const RectOrderInfo& orderInfo)
			: highOrder((static_cast<uint64_t>(orderInfo.displayOrder) << 32)
					| ((static_cast<uint64_t>(orderInfo.screenIndex) & 0xFF'FF) << 16))
			, lowOrder(0) {
		if (orderInfo.zIndexBehavior == Game::ZIndexBehavior::GLOBAL) {
			highOrder |= (static_cast<uint64_t>(orderInfo.zIndex) >> 16) & 0xFF'FF;
			lowOrder = (static_cast<uint64_t>(orderInfo.zIndex) & 0xFF'FF) << 48
					| ((static_cast<uint64_t>(orderInfo.depth) & 0xFF'FF) << 32)
					| ((static_cast<uint64_t>(orderInfo.childIndex) & 0xFF'FF'FF'FE) << 1)
					| static_cast<uint64_t>(orderInfo.overlay);
		}
		else {
			highOrder |= static_cast<uint64_t>(orderInfo.depth) & 0xFF'FF;
			lowOrder = (static_cast<uint64_t>(orderInfo.zIndex) << 32)
					| ((static_cast<uint64_t>(orderInfo.childIndex) & 0xFF'FF'FF'FE) << 1)
					| static_cast<uint64_t>(orderInfo.overlay);
		}
	}

	bool operator==(const RectPriority& other) const {
		return highOrder == other.highOrder && lowOrder == other.lowOrder;
	}

	bool operator!=(const RectPriority& other) const {
		return highOrder != other.highOrder || lowOrder != other.lowOrder;
	}

	bool operator<(const RectPriority& other) const {
		return highOrder < other.highOrder
				|| (highOrder == other.highOrder && lowOrder < other.lowOrder);
	}
};

struct RectInstanceInfo {
	Math::Vector2 position;
	RectPriority priority;

	bool operator<(const RectInstanceInfo& other) const {
		return priority < other.priority;
	}
};

}

