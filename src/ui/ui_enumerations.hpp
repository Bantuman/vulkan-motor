#pragma once

#include <cstdint>

namespace Game {

enum class ZIndexBehavior : uint8_t {
	GLOBAL = 0,
	SIBLING = 1
};

enum class ResizeDirection : uint8_t {
	LEFT = 0,
	RIGHT = 1,
	UP = 2,
	DOWN = 3
};

enum class TextXAlignment : uint8_t {
	LEFT = 0,
	RIGHT = 1,
	CENTER = 2
};

enum class TextYAlignment : uint8_t {
	TOP = 0,
	CENTER = 1,
	BOTTOM = 2
};

enum class TextTruncate : uint8_t {
	NONE = 0,
	AT_END = 1
};

enum class ScaleType : uint8_t {
	STRETCH = 0,
	SLICE = 1,
	TILE = 2,
	FIT = 3,
	CROP = 4
};

enum class ResamplerMode : uint8_t {
	DEFAULT = 0, // LINEAR
	PIXELATED = 1 // NEAREST
};

enum class BorderMode : uint8_t {
	OUTLINE = 0,
	MIDDLE = 1,
	INSET = 2
};

enum class SizeConstraint : uint8_t {
	RELATIVE_XY = 0,
	RELATIVE_XX = 1,
	RELATIVE_YY = 2
};

enum class AutomaticSize : uint8_t {
	NONE = 0,
	X = 1,
	Y = 2,
	XY = 3
};

enum class FrameStyle : uint8_t {
	CUSTOM = 0,
	CHAT_BLUE = 1,
	SQUARE = 2,
	ROUND = 3,
	CHAT_GREEN = 4,
	CHAT_RED = 5,
	DROP_SHADOW = 6
};

enum class ButtonStyle : uint8_t {
	CUSTOM = 0,
	BUTTON_DEFAULT = 1,
	BUTTON = 2,
	ROUND_BUTTON = 3,
	ROUND_DEFAULT_BUTTON = 4,
	ROUND_DROPDOWN_BUTTON = 5
};

}

