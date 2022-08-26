#include "context_action.hpp"

using namespace Game;

// ContextKey

bool ContextActionManager::ContextKey::operator==(const ContextKey& other) const {
	return type == other.type && keyCode == other.keyCode;
}

size_t ContextActionManager::ContextKey::hash() const {
	return static_cast<size_t>(type) | (static_cast<size_t>(keyCode) << 8);
}

// Action

bool ContextActionManager::Action::operator<(const Action& other) const {
	return priority < other.priority;
}

// ContextActionManager

void ContextActionManager::bind_action(std::string_view name, UserInputType type, int keyCode,
		CallbackObject&& cb) {
	bind_action_at_priority(std::move(name), type, keyCode, ContextActionPriority::DEFAULT,
			std::move(cb));
}

void ContextActionManager::bind_action_at_priority(std::string_view name, UserInputType type,
		int keyCode, ContextActionPriority priority, CallbackObject&& cb) {
	if (type != UserInputType::KEYBOARD) {
		keyCode = 0;
	}

	ContextKey key{type, keyCode};
	Action newAction{cb, priority, std::string(std::move(name))};

	auto& actions = m_bindings[key];

	for (ptrdiff_t i = static_cast<ptrdiff_t>(actions.size() - 1); i >= 0; --i) {
		if (actions[i] < newAction) {
			actions.insert(actions.begin() + i, std::move(newAction));
			return;
		}
	}

	actions.push_back(std::move(newAction));
}

void ContextActionManager::unbind_action(std::string_view name) {
	for (auto& [_, actions] : m_bindings) {
		for (auto it = actions.begin(), end = actions.end(); it != end; ++it) {
			if (it->name.compare(name) == 0) {
				actions.erase(it);
				return;
			}
		}
	}
}

void ContextActionManager::unbind_all_actions() {
	m_bindings.clear();
}

void ContextActionManager::fire_input(const InputObject& inputObject) {
	ContextKey key{inputObject.type, inputObject.keyCode};
	auto& actions = m_bindings[key];

	for (auto it = actions.rbegin(), end = actions.rend(); it != end; ++it) {
		if (it->callback(inputObject) == ContextActionResult::SINK) {
			return;
		}
	}
}

