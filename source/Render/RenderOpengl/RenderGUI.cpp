#include "RenderGUI.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include <string>
#include <iostream>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std::literals;

namespace
{
	void glfwSetWindowCenter(GLFWwindow* window) {
		// Get window position and size
		int window_x, window_y;
		glfwGetWindowPos(window, &window_x, &window_y);

		int window_width, window_height;
		glfwGetWindowSize(window, &window_width, &window_height);

		// Halve the window size and use it to adjust the window position to the center of the window
		window_width *= 0.5;
		window_height *= 0.5;

		window_x += window_width;
		window_y += window_height;

		// Get the list of monitors
		int monitors_length;
		GLFWmonitor** monitors = glfwGetMonitors(&monitors_length);

		if (monitors == NULL) {
			// Got no monitors back
			return;
		}

		// Figure out which monitor the window is in
		GLFWmonitor* owner = NULL;
		int owner_x, owner_y, owner_width, owner_height;

		for (int i = 0; i < monitors_length; i++) {
			// Get the monitor position
			int monitor_x, monitor_y;
			glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

			// Get the monitor size from its video mode
			int monitor_width, monitor_height;
			GLFWvidmode* monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

			if (monitor_vidmode == NULL) {
				// Video mode is required for width and height, so skip this monitor
				continue;

			}
			else {
				monitor_width = monitor_vidmode->width;
				monitor_height = monitor_vidmode->height;
			}

			// Set the owner to this monitor if the center of the window is within its bounding box
			if ((window_x > monitor_x && window_x < (monitor_x + monitor_width)) && (window_y > monitor_y && window_y < (monitor_y + monitor_height))) {
				owner = monitors[i];

				owner_x = monitor_x;
				owner_y = monitor_y;

				owner_width = monitor_width;
				owner_height = monitor_height;
			}
		}

		if (owner != NULL) {
			// Set the window position to the center of the owner monitor
			glfwSetWindowPos(window, owner_x + (owner_width * 0.5) - window_width, owner_y + (owner_height * 0.5) - window_height);
		}
	}
}

class RenderGUI::Impl
{	
public:
	Impl()
	{
	}

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		auto _this = reinterpret_cast<RenderGUI::Impl*>(glfwGetWindowUserPointer(window));
		glViewport(0, 0, width, height);
	}

	~Impl()
	{
	}

	void init()
	{
		initWindow();

		glEnable(GL_MULTISAMPLE);
		glEnable(GL_FRAMEBUFFER_SRGB);
	}

	void initWindow()
	{
		if (!glfwInit())
			throw std::runtime_error{ "glfwInit has failed" };

		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 8);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(561, 585, " ", NULL, NULL);
		if (!m_window)
			throw std::runtime_error{ "glfwCreateWindow has failed" };

		glfwSetWindowCenter(m_window);

		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(0);

		glfwSetWindowUserPointer(m_window, this);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
			throw std::runtime_error{ "gladLoadGLLoader has failed" };

		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int x, int y) {
			auto _this = reinterpret_cast<RenderGUI::Impl*>(glfwGetWindowUserPointer(window));
			std::cout << "X: " << x << " Y: " << y << std::endl;
				glViewport(0, 0, x, y);
				// render(); 
		});
	}

	RenderGuiData startRenderLoop(RenderGuiData result)
	{		
		init();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glClearColor(1.f, 1.f, 1.f, 1.0f);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 130");


	    if (result.averageFps > 0)
		{
			renderResultMenu(result);
		}

		result = renderMainMenu(result);

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(m_window);

		return result;
	}

	RenderGuiData renderMainMenu(RenderGuiData result)
	{

		bool cbSimple = result.simpleScene;
		bool cbComplex = !result.simpleScene;

		bool cbVulkan = result.renderType == RenderGuiData::RenderType::Vulkan;
		bool cbOpengl = result.renderType == RenderGuiData::RenderType::OpenGL;

		bool cbHigh = result.sceneLoad == RenderGuiData::SceneLoad::High;
		bool cbMedium = result.sceneLoad == RenderGuiData::SceneLoad::Med;
		bool cbLow = result.sceneLoad == RenderGuiData::SceneLoad::Low;


		int modelNumber = result.modelNumber;
		while (true)
		{
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(m_window))
			{
				result.renderType = RenderGuiData::RenderType::Exit;
				break;
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin(" ");

			ImGui::SetWindowFontScale(3.5);
			ImGui::Button("RENDER TYPE");

			if (ImGui::Checkbox("VULKAN", &cbVulkan))
			{
				cbVulkan = true;
				cbOpengl = false;
			}

			if (ImGui::Checkbox("OPENGL", &cbOpengl))
			{
				cbVulkan = false;
				cbOpengl = true;
			}

			ImGui::Button("RENDER SCENE");

			if (ImGui::Checkbox("SIMPLE", &cbSimple))
			{
				cbSimple = true;
				cbComplex = false;
			}

			if (ImGui::Checkbox("COMPLEX", &cbComplex))
			{
				cbSimple = false;
				cbComplex = true;
			}

			ImGui::Button("MODEL NUMBER");

			if (ImGui::InputInt("1-512", &modelNumber, 10, 512))
			{
				cbHigh = false;
				cbMedium = false;
				cbLow = false;
			}

			if (modelNumber < 1)
				modelNumber = 1;
			else if (modelNumber > 512)
				modelNumber = 512;

			

			ImVec2 windowSize = ImGui::GetIO().DisplaySize;

			ImGui::BeginChild(1, { 50, 50 });
			ImGui::EndChild();

			ImGui::SetCursorPosX(180);
			ImGui::BeginChild(2);
			ImGui::SetWindowFontScale(1.5);
			bool startPressed = ImGui::Button("START");

			ImGui::EndChild();
			ImGui::End();



			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


			glfwSwapBuffers(m_window);
			glfwPollEvents();

			if (startPressed)
				break;
		}

		if (result.renderType != RenderGuiData::RenderType::Exit)
		{
			if (cbOpengl)
				result.renderType = RenderGuiData::RenderType::OpenGL;
			else if (cbVulkan)
				result.renderType = RenderGuiData::RenderType::Vulkan;
		}

		if (cbHigh)
			result.sceneLoad = RenderGuiData::SceneLoad::High;
		else if (cbMedium)
			result.sceneLoad = RenderGuiData::SceneLoad::Med;
		else if (cbLow)
			result.sceneLoad = RenderGuiData::SceneLoad::Low;

		result.modelNumber = modelNumber;

		result.simpleScene = cbSimple;

		return result;
	}

	void renderResultMenu(RenderGuiData result)
	{
		while (true)
		{
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(m_window))
			{
				result.renderType = RenderGuiData::RenderType::Exit;
				break;
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin(" ");

			ImGui::SetWindowFontScale(3.5);

			ImGui::BeginChild(22, { 50, 5 });
			ImGui::EndChild();

			ImGui::Button("  AVERAGE FPS (1/SPF)          ");

			ImGui::BeginChild(1, { 50, 25 });
			ImGui::EndChild();

			double res = result.averageFps;
			ImGui::SetCursorPosX(110);
			ImGui::InputDouble("  ", &res, 0.0, 0.0, "%.3f");

			ImGui::BeginChild(1543, { 50, 15 });
			ImGui::EndChild();


			ImGui::Button("  AVERAGE SPF (1/FPS)            ");

			ImGui::BeginChild(63241, { 50, 15 });
			ImGui::EndChild();

			double res2 = 1/result.averageFps;
			ImGui::SetCursorPosX(110);
			ImGui::InputDouble(" ", &res2, 0.0, 0.0, "%.5f");

			ImGui::BeginChild(63212341, { 50, 15 });
			ImGui::EndChild();

			ImGui::Button("    CORES NUMBER            ");

			ImGui::BeginChild(6324321, { 50, 15 });
			ImGui::EndChild();

			int res3 = std::thread::hardware_concurrency();
			ImGui::SetCursorPosX(110);
			ImGui::InputInt("   ", &res3, 0, 0);

			ImGui::BeginChild(9876543, { 50, 15 });
			ImGui::EndChild();

			ImGui::SetCursorPosX(180);
			ImGui::BeginChild(2);
			ImGui::SetWindowFontScale(1.5);
			bool startPressed = ImGui::Button(" BACK ");

			ImGui::EndChild();
			ImGui::End();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


			glfwSwapBuffers(m_window);
			glfwPollEvents();

			if (startPressed)
				break;
		}
	}
private:
	GLFWwindow* m_window;
};

RenderGUI::RenderGUI()
	: m_impl{ std::make_unique< RenderGUI::Impl>() }
{

}

RenderGUI::~RenderGUI() = default;

RenderGuiData RenderGUI::startRenderLoop(RenderGuiData result)
{
	return m_impl->startRenderLoop(result);
}


