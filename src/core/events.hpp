#pragma once

#include <cstdint>
#include <functional>

namespace Event {

template <typename... Args>
class Dispatcher {
	public:
		using function_type = void(Args...);

		class Connection {
			template <typename Functor>
			explicit Connection(Functor&& func, Connection* next)
					: m_function(func)
					, m_next(next) {}

			std::function<function_type> m_function;
			Connection* m_next;

			friend class Dispatcher;
		};

		~Dispatcher() {
			auto* curr = m_head;

			while (curr) {
				auto* next = curr->m_next;
				delete curr;
				curr = next;
			}
		}

		template <typename Functor>
		Connection* connect(Functor&& func) {
			m_head = new Connection(func, m_head);
			return m_head;
		}

		void disconnect(Connection* con) {
			if (con == m_head) {
				m_head = m_head->m_next;
				delete con;
				return;
			}

			auto* last = m_head;
			auto* curr = m_head->m_next;

			while (curr) {
				if (curr == con) {
					last->m_next = con->m_next;
					delete con;
					return;
				}

				last = curr;
				curr = curr->m_next;
			}
		}

		template <typename... Args2>
		void fire(Args2&&... args) {
			auto* curr = m_head;

			while (curr) {
				curr->m_function(std::forward<Args2>(args)...);
				curr = curr->m_next;
			}
		}

		bool empty() const {
			return m_head == nullptr;
		}
	private:
		Connection* m_head = nullptr;
};

}

