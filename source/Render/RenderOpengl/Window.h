#include "GLFW/glfw3.h"
#include <functional>

class Window
{
public:
	Window();
	~Window();

	using resizeCallback_t = std::function<void(GLFWwindow*, int, int)>;
	using mouseCallback_t = std::function<void(GLFWwindow*, double, double)>;

	GLFWwindow* get();

	void setResizeCallback(resizeCallback_t resizeCallback);
	void setMouseCallback(mouseCallback_t resizeCallback);
	void setMouseScrollCallback(mouseCallback_t resizeCallback);

	const resizeCallback_t& getResizeCallback();
	const mouseCallback_t& getMouseCallback();
	const mouseCallback_t& getMouseScrollCallback();
private:
	GLFWwindow* m_window = nullptr;
	resizeCallback_t m_resizeCallback = [] (auto...) { };
	mouseCallback_t m_mouseCallback = [](auto...) {};
	mouseCallback_t m_mouseScrollCallback = [](auto...) {};
};