#include "application.hpp"
#include "GLFW/glfw3.h"

#include <core/logging.hpp>

#include <core/context_action.hpp>

#include <cassert>
#include <cstring>
#include <atomic>

static std::atomic_size_t g_instanceCount{0};

static constexpr size_t KEY_COUNT = GLFW_KEY_LAST + 1;
static constexpr size_t MOUSE_BUTTON_COUNT = GLFW_MOUSE_BUTTON_LAST + 1;

static bool g_keyStates[KEY_COUNT] = {};
static bool g_mouseButtonStates[MOUSE_BUTTON_COUNT] = {};

static double g_cursorX = 0;
static double g_cursorY = 0;

static double g_offsetX = 0;
static double g_offsetY = 0;

static double g_deltaOffsetX = 0;
static double g_deltaOffsetY = 0;

static double g_deltaX = 0;
static double g_deltaY = 0;

static Application::KeyPressEvent g_keyDownEvent;
static Application::KeyPressEvent g_keyUpEvent;
static Application::MouseClickEvent g_mouseDownEvent;
static Application::MouseClickEvent g_mouseUpEvent;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
static void scrollCallback(GLFWwindow*, double xOffset, double yOffset);
static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

Application::Application() noexcept {
	if (g_instanceCount.fetch_add(1, std::memory_order_relaxed) == 0) {
		if (!glfwInit()) {
			LOG_ERROR2("GLFW", "Failed to initialize GLFW");
		}
	}
}

Application::~Application() noexcept {
	if (g_instanceCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
		glfwTerminate();
	}
}
void Application::reset_deltas() noexcept {
	g_deltaOffsetX = 0;
	g_deltaOffsetY = 0;
	g_offsetX = 0;
	g_offsetY = 0;
}

void Application::poll_events() noexcept {
	g_deltaX = 0;
	g_deltaY = 0;

	glfwPollEvents();
}

bool Application::is_key_down(int key) const {
	return g_keyStates[key];
}

bool Application::is_mouse_button_down(int mouseButton) const {
	return g_mouseButtonStates[mouseButton];
}

double Application::get_time() const {
	return glfwGetTime();
}

double Application::get_mouse_x() const {
	return g_cursorX;
}

double Application::get_mouse_y() const {
	return g_cursorY;
}

double Application::get_scroll_x() const
{
	return g_offsetX;
}

double Application::get_scroll_y() const
{
	return g_offsetY;
}

double Application::get_scroll_delta_x() const
{
	return g_deltaOffsetX;
}

double Application::get_scroll_delta_y() const
{
	return g_deltaOffsetY;
}

Math::Vector2 Application::get_mouse_position() const {
	return Math::Vector2(static_cast<float>(g_cursorX), static_cast<float>(g_cursorY));
}

double Application::get_mouse_delta_x() const {
	return g_deltaX;
}

double Application::get_mouse_delta_y() const {
	return g_deltaY;
}

Application::KeyPressEvent& Application::key_down_event() {
	return g_keyDownEvent;
}

Application::KeyPressEvent& Application::key_up_event() {
	return g_keyUpEvent;
}

Window::Window(int width, int height, const char* title) noexcept
		: m_handle(nullptr)
		, m_width(width)
		, m_height(height) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);

	if (!m_handle) {
		LOG_ERROR2("GLFW", "Failed to create window");
	}

	glfwSetWindowUserPointer(m_handle, this);

	glfwGetCursorPos(m_handle, &g_cursorX, &g_cursorY);

	glfwSetKeyCallback(m_handle, keyCallback);
	glfwSetScrollCallback(m_handle, scrollCallback);
	glfwSetMouseButtonCallback(m_handle, mouseButtonCallback);
	glfwSetCursorPosCallback(m_handle, cursorPosCallback);
	glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);
}

Window::~Window() noexcept {
	glfwDestroyWindow(m_handle);
}

void Window::swap_buffers() noexcept {
	assert(m_handle);
	glfwSwapBuffers(m_handle);
}

bool Window::is_close_requested() const noexcept {
	assert(m_handle);
	return glfwWindowShouldClose(m_handle);
}

int Window::get_width() const noexcept {
	assert(m_handle);
	return m_width;
}

int Window::get_height() const noexcept {
	assert(m_handle);
	return m_height;
}

double Window::get_aspect_ratio() const noexcept {
	assert(m_handle);
	return static_cast<double>(m_width) / static_cast<double>(m_height);
}

GLFWwindow* Window::get_handle() noexcept {
	return m_handle;
}

Window::ResizeEvent& Window::resize_event() {
	return m_resizeEvent;
}

void Window::set_size(int width, int height) {
	if (width != 0 && height != 0) {
		m_width = width;
		m_height = height;
	}

	m_resizeEvent.fire(width, height);
}

static void keyCallback(GLFWwindow* handle, int key, int scancode, int action, int mods) {
	(void)handle;
	(void)scancode;
	(void)mods;

	bool keyState = action != GLFW_RELEASE;

	if (keyState != g_keyStates[key]) {
		Game::InputObject io{};
		io.keyCode = key;
		io.type = Game::UserInputType::KEYBOARD;

		if (keyState) {
			io.state = Game::UserInputState::BEGIN;
			g_keyDownEvent.fire(key);
		}
		else {
			io.state = Game::UserInputState::END;
			g_keyUpEvent.fire(key);
		}

		g_contextActionManager->fire_input(io);
	}

	g_keyStates[key] = keyState;
}

static void cursorPosCallback(GLFWwindow*, double xPos, double yPos) {
	g_deltaX = xPos - g_cursorX;
	g_deltaY = yPos - g_cursorY;

	g_cursorX = xPos;
	g_cursorY = yPos;
}

static void scrollCallback(GLFWwindow*, double xOffset, double yOffset)
{
	g_deltaOffsetX = xOffset - g_offsetX;
	g_deltaOffsetY = yOffset - g_offsetY;

	g_offsetX = xOffset;
	g_offsetY = yOffset;
}

static void mouseButtonCallback(GLFWwindow* handle, int button, int action, int mods) {
	(void)handle;
	(void)mods;

	bool buttonState = action != GLFW_RELEASE;

	if (buttonState != g_mouseButtonStates[button]) {
		Game::InputObject io{};
		io.position = Math::Vector3(static_cast<float>(g_cursorX), static_cast<float>(g_cursorY),
				0.f);

		switch (button) {
			case GLFW_MOUSE_BUTTON_1:
				io.type = Game::UserInputType::MOUSE_BUTTON_1;
				break;
			case GLFW_MOUSE_BUTTON_2:
				io.type = Game::UserInputType::MOUSE_BUTTON_2;
				break;
			case GLFW_MOUSE_BUTTON_3:
				io.type = Game::UserInputType::MOUSE_BUTTON_3;
				break;
			default:
				io.type = Game::UserInputType::NONE;
		}

		if (buttonState) {
			io.state = Game::UserInputState::BEGIN;
			g_mouseDownEvent.fire(button);
		}
		else {
			io.state = Game::UserInputState::END;
			g_mouseUpEvent.fire(button);
		}

		g_contextActionManager->fire_input(io);
	}

	g_mouseButtonStates[button] = buttonState;
}

static void framebufferSizeCallback(GLFWwindow* handle, int width, int height) {
	Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(handle));
	window->set_size(width, height);
}

//static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
//}

