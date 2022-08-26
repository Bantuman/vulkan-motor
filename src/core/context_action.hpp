#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <core/local.hpp>

#include <core/user_input.hpp>

namespace Game {

enum class ContextActionResult {
	SINK = 0,
	PASS = 1
};

enum class ContextActionPriority {
	LOW = 1000,
	MEDIUM = 2000,
	HIGHT = 3000,

	DEFAULT = MEDIUM
};

using ContextActionCallback = ContextActionResult(const InputObject&);

class ContextActionManager {
	public:
		using CallbackObject = std::function<ContextActionCallback>;

		void bind_action(std::string_view name, UserInputType, int keyCode, CallbackObject&&);
		void bind_action_at_priority(std::string_view name, UserInputType, int keyCode,
				ContextActionPriority, CallbackObject&&);

		void unbind_action(std::string_view name);
		void unbind_all_actions();

		void fire_input(const InputObject&);
	private:
		struct ContextKey {
			UserInputType type;
			int keyCode;

			bool operator==(const ContextKey&) const;
			size_t hash() const;
		};

		struct ContextKeyHash {
			size_t operator()(const ContextKey& k) const {
				return k.hash();
			}
		};

		struct Action {
			CallbackObject callback;
			ContextActionPriority priority;
			std::string name;

			bool operator<(const Action&) const;
		};

		std::unordered_map<ContextKey, std::vector<Action>, ContextKeyHash> m_bindings;
};

}

inline Local<Game::ContextActionManager> g_contextActionManager;

