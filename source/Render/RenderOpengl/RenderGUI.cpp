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
		m_window = glfwCreateWindow(561, 585, "My Title", NULL, NULL);
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

	RenderGuiData  startRenderLoop()
	{		
		init();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glClearColor(1.f, 1.f, 1.f, 1.0f);

		RenderGuiData result;

		result.renderType = RenderGuiData::RenderType::Exit;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 130");



		bool cbVulkan = false;
		bool cbOpengl = false;

		bool cbHigh= false;
		bool cbMedium = false;
		bool cbLow = false;

        while (!glfwWindowShouldClose(m_window))
        {
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
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


			if (ImGui::Checkbox("Hight (100)", &cbHigh))
			{
				cbHigh = true;
				cbMedium = false;
				cbLow = false;
			}

			if (ImGui::Checkbox("Medium (50)", &cbMedium))
			{
				cbHigh = false;
				cbMedium = true;
				cbLow = false;
			}

			if (ImGui::Checkbox("Low (10)", &cbLow))
			{
				cbHigh = false;
				cbMedium = false;
				cbLow = true;
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

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (cbOpengl)
			result.renderType = RenderGuiData::RenderType::OpenGL;
		else if(cbVulkan)
			result.renderType = RenderGuiData::RenderType::Vulkan;

		if(cbHigh)
			result.sceneLoad = RenderGuiData::SceneLoad::High;
		else if(cbMedium)
			result.sceneLoad = RenderGuiData::SceneLoad::Med;
		else if(cbLow)
			result.sceneLoad = RenderGuiData::SceneLoad::Low;

		glfwDestroyWindow(m_window);

		return result;
	}
private:
	GLFWwindow* m_window;
};

RenderGUI::RenderGUI()
	: m_impl{ std::make_unique< RenderGUI::Impl>() }
{

}

RenderGUI::~RenderGUI() = default;

RenderGuiData RenderGUI::startRenderLoop()
{
	return m_impl->startRenderLoop();
}
