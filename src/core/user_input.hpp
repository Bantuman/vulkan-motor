#pragma once

#include <cstdint>

#include <math/vector3.hpp>

namespace Game {

enum class UserInputType : uint8_t {
	MOUSE_BUTTON_1 = 0,
	MOUSE_BUTTON_2 = 1,
	MOUSE_BUTTON_3 = 2,
	MOUSE_WHEEL = 3,
	MOUSE_MOVEMENT = 4,
	TOUCH = 7, // Unused, mobile
	KEYBOARD = 8,
	FOCUS = 9, // Focus regained to window
	ACCELEROMETER = 10, // Unused, mobile
	GYRO = 11, // Unused, mobile
	GAMEPAD_1 = 12,
	GAMEPAD_2 = 13,
	GAMEPAD_3 = 14,
	GAMEPAD_4 = 15,
	GAMEPAD_5 = 16,
	GAMEPAD_6 = 17,
	GAMEPAD_7 = 18,
	GAMEPAD_8 = 19,
	TEXT_INPUT = 20, // Input of text into a text-based GuiObject. Normally this is only a TextBox
	INPUT_METHOD = 21, // Text input from an input method editor (IME), unsupported
	NONE = 22
};

enum class UserInputState : uint8_t {
	BEGIN = 0,
	CHANGE = 1, // Occurs each frame an inputobject has already begun interacting with the game
				// and part of its state is changing, for example a mouse movement/gamepad analog
	END = 2,
	CANCEL = 3, // Indicates this input is no longer relevant, e.g. binding two action-handling
				// functions will cause the first to cancel if an inpu was already in progress
				// when the second was bound
	NONE = 4 // Should never be encountered, marks end of enum
};

enum class ModifierKey : uint8_t {
	SHIFT = 0,
	CTRL = 1,
	ALT = 2,
	META = 3
};

struct InputObject {
	Math::Vector3 delta;
	Math::Vector3 position;
	int keyCode; // FIXME: enum KeyCode
	UserInputState state;
	UserInputType type;

	// FIXME: bool is_modifier_key_down(ModifierKey) const;
};

}

