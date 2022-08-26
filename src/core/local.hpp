#pragma once

#include <cassert>
#include <cstdint>

#include <new>
#include <utility>

template <typename T>
class Local {
	public:
		Local() noexcept = default;

		Local(const Local&) = delete;
		void operator=(const Local&) = delete;

		Local(Local&&) = delete;
		void operator=(Local&&) = delete;

		template <typename... Args>
		void create(Args&&... args) noexcept {
			assert(!ptr);
			ptr = new (data) T(std::forward<Args>(args)...);
		}

		void destroy() noexcept {
			assert(ptr);
			ptr->~T();
			ptr = nullptr;
		}

		T& operator*() noexcept {
			assert(ptr);
			return *ptr;
		}

		T* operator->() const noexcept {
			assert(ptr);
			return ptr;
		}

		T* get() const noexcept {
			return ptr;
		}

		operator bool() const noexcept {
			return ptr != nullptr;
		}

		~Local() noexcept {
			if (ptr) {
				destroy();
			}
		}
	private:
		alignas(T) uint8_t data[sizeof(T)];
		T* ptr = nullptr;
};

