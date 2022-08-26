#pragma once

#include <memory>

namespace Memory {

// Waiting for C++20
template <typename T, typename... Args>
T* construct_at(T* p, Args&&... args) {
	return ::new (const_cast<void*>(static_cast<const volatile void*>(p)))
			T(std::forward<Args>(args)...);
}

using std::uninitialized_move;
using std::destroy;
using std::destroy_at;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

using std::static_pointer_cast;

}

