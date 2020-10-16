#include "glad/glad.h"
#include "Window.h"
#include <stdexcept>
#include <iostream>
#include <string>

static void errorCcallback(int error, const char* description)
{
	std::cerr << "glfwError #" << error << " ; desc: " << description << std::endl;
}

static void APIENTRY logCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
	auto source_str = [source]() -> std::string {
		switch (source)
		{
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY:  return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";
		default: return "UNKNOWN";
		}
	}();

	auto type_str = [type]() {
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER:  return "MARKER";
		case GL_DEBUG_TYPE_OTHER: return "OTHER";
		default: return "UNKNOWN";
		}
	}();

	auto severity_str = [severity]() {
		switch (severity) {
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW: return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
		default: return "UNKNOWN";
		}
	}();

	std::cout << source_str << ", "
		<< type_str << ", "
		<< severity_str << ", "
		<< id << ": "
		<< message << std::endl;
}

Window::Window()
{
	if (!glfwInit())
		throw std::runtime_error{ "glfwInit has failed" };


	glfwSetErrorCallback(errorCcallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(800, 600, "My Title", NULL, NULL);
	if (!m_window)
		throw std::runtime_error{ "glfwCreateWindow has failed" };

	glfwMakeContextCurrent(m_window);

	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Unlimited fps
	//glfwSwapInterval(0);

	glfwSetWindowUserPointer(m_window, this);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
		throw std::runtime_error{ "gladLoadGLLoader has failed" };

	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int x, int y) {
		auto _this = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		_this->getResizeCallback()(window, x, y);
	});

	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
		auto _this = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		_this->getMouseCallback()(window, x, y);
	});

	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double x, double y) {
		auto _this = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		_this->getMouseScrollCallback()(window, x, y);
	});

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(logCallback, nullptr);
}

Window::~Window()
{
	glfwTerminate();
}

GLFWwindow* Window::get()
{
	return m_window;
}

void Window::setResizeCallback(resizeCallback_t resizeCallback)
{
	m_resizeCallback = resizeCallback;
}

void Window::setMouseCallback(mouseCallback_t resizeCallback)
{
	m_mouseCallback = resizeCallback;
}

void Window::setMouseScrollCallback(mouseCallback_t resizeCallback)
{
	m_mouseScrollCallback = resizeCallback;
}

const Window::resizeCallback_t& Window::getResizeCallback()
{
	return m_resizeCallback;
}

const Window::mouseCallback_t& Window::getMouseCallback()
{
	return m_mouseCallback;
}

const Window::mouseCallback_t& Window::getMouseScrollCallback()
{
	return m_mouseScrollCallback;
}
