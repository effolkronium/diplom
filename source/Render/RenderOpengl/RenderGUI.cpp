#include "RenderGUI.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include <string>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std::literals;

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


			ImGui::Button("MODEL NUMBER");

			if (ImGui::InputInt("10-500", &modelNumber, 10, 500))
			{
				cbHigh = false;
				cbMedium = false;
				cbLow = false;
			}

			if (modelNumber < 10)
				modelNumber = 10;
			else if (modelNumber > 500)
				modelNumber = 500;

			if (ImGui::Checkbox("Hight (500)", &cbHigh) || modelNumber == 500)
			{
				cbHigh = true;
				cbMedium = false;
				cbLow = false;
				modelNumber = 500;
			}

			if (ImGui::Checkbox("Medium (100)", &cbMedium) || modelNumber == 100)
			{
				cbHigh = false;
				cbMedium = true;
				cbLow = false;
				modelNumber = 100;
			}

			if (ImGui::Checkbox("Low (10)", &cbLow) || modelNumber == 10)
			{
				cbHigh = false;
				cbMedium = false;
				cbLow = true;
				modelNumber = 10;
			}

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

			ImGui::BeginChild(22, { 50, 40 });
			ImGui::EndChild();

			ImGui::Button("  AVERAGE FPS (1/SPF)          ");

			ImGui::BeginChild(1, { 50, 25 });
			ImGui::EndChild();

			double res = result.averageFps;
			ImGui::SetCursorPosX(110);
			ImGui::InputDouble(" ", &res, 0.0, 0.0, "%.3f");

			ImGui::BeginChild(1543, { 50, 50 });
			ImGui::EndChild();


			ImGui::Button("  AVERAGE SPF (1/FPS)            ");

			ImGui::BeginChild(63241, { 50, 25 });
			ImGui::EndChild();

			double res2 = 1/result.averageFps;
			ImGui::SetCursorPosX(110);
			ImGui::InputDouble(" ", &res2, 0.0, 0.0, "%.3f");

			ImGui::BeginChild(15453, { 50, 100 });
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


