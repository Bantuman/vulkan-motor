#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <core/local.hpp>
#include <core/events.hpp>

#include <math/vector2.hpp>

class Application final {
	public:
		using KeyPressEvent = Event::Dispatcher<int>;
		using MouseClickEvent = Event::Dispatcher<int>;

		explicit Application() noexcept;
		~Application() noexcept;

		void poll_events() noexcept;
		void reset_deltas() noexcept;

		bool is_key_down(int key) const;
		bool is_mouse_button_down(int mouseButton) const;

		double get_time() const;

		double get_mouse_x() const;
		double get_mouse_y() const;

		double get_scroll_x() const;
		double get_scroll_y() const;

		double get_scroll_delta_x() const;
		double get_scroll_delta_y() const;

		Math::Vector2 get_mouse_position() const;

		double get_mouse_delta_x() const;
		double get_mouse_delta_y() const;

		KeyPressEvent& key_down_event();
		KeyPressEvent& key_up_event();

		MouseClickEvent& mouse_down_event();
		MouseClickEvent& mouse_up_event();
	private:
};

class Window final {
	public:
		using ResizeEvent = Event::Dispatcher<int, int>;

		explicit Window(int width, int height, const char* title) noexcept;
		~Window() noexcept;

		void swap_buffers() noexcept;

		bool is_close_requested() const noexcept;

		int get_width() const noexcept;
		int get_height() const noexcept;
		double get_aspect_ratio() const noexcept;

		GLFWwindow* get_handle() noexcept;

		void set_size(int width, int height);

		ResizeEvent& resize_event();
	private:
		GLFWwindow* m_handle;

		int m_width;
		int m_height;

		ResizeEvent m_resizeEvent;
};

inline Local<Application> g_application;
inline Local<Window> g_window;
