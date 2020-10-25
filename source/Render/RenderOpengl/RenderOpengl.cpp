#include "RenderOpengl.h"

#include "glad/glad.h"
#include "Window.h"
#include "GLFW/glfw3.h"
#include "Shader.h"
#include "Shaders.h"
#include "Camera.h"

#include <string>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

using namespace std::literals;

class RenderOpengl::Impl
{
public:
	float fov = 45.f;
	Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) };

	Impl()
	{
		m_window.setResizeCallback([this] (auto w, int x, int y) {
			glViewport(0, 0, x, y);
			// render(); 
		});

		m_window.setMouseCallback([this](auto w, double xpos, double ypos) {
			static float lastX = 400, lastY = 300;
			static bool firstMouse = true;
			if (firstMouse)
			{
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			float xoffset = xpos - lastX;
			float yoffset = lastY - ypos;
			lastX = xpos;
			lastY = ypos;

			camera.ProcessMouseMovement(xoffset, yoffset);
		});

		m_window.setMouseScrollCallback([this](auto w, double xpos, double ypos) {
			fov -= (float)ypos;
			if (fov < 1.0f)
				fov = 1.0f;
			if (fov > 45.0f)
				fov = 45.0f;
		});
	}

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		auto _this = reinterpret_cast<RenderOpengl::Impl*>(glfwGetWindowUserPointer(window));
		glViewport(0, 0, width, height);
		//_this->render();
	}

	~Impl()
	{
	}

	void startRenderLoop()
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		Shader ourShader(s_model_loading, s_model_loading_v);

		ourShader.use();
		ourShader.setInt("material.texture_diffuse1", 0);
		ourShader.setInt("material.texture_specular1", 1);

		//Model ourModel("resources/bird/Bird.FBX", { "resources/bird/Fogel_Mat_Diffuse_Color.png" });

		/*Model ourModel("resources/tiger/Tiger.fbx", { 
			"resources/tiger/Tiger.fbm/FbxTemp_0001.jpg",
			"resources/tiger/Tiger.fbm/FbxTemp_0002.jpg",
		});*/

		/*Model ourModel("resources/chimp/chimp.FBX", {
			"resources/chimp/chimp_diffuse.jpg",
			"resources/chimp/chimp_normal.jpg",
			"resources/chimp/chimp_spec.jpg",
		});*/

		/*Model ourModel(
			"resources/crocodile/Crocodile.fbx", {
			"resources/crocodile/Crocodile.jpg",
			"resources/crocodile/CrocodileBM.jpg",
		});*/

		/*Model ourModel(
			"resources/penguin/penguin.FBX", {
			"resources/penguin/qi e.png",
		});*/

		Model ourModel(
			"resources/shetlandponyamber/ShetlandPonyAmberM.fbx", {
			"resources/shetlandponyamber/shetlandponyamber.png",
		});


		ourShader.setVec3("dirLight2.direction", { +1.f, 1.f, 1.f });
		ourShader.setVec3("dirLight2.ambient", { 0.15f, 0.15f, 0.15f, });
		ourShader.setVec3("dirLight2.diffuse", { 0.5f, 0.5f, 0.5f });
		ourShader.setVec3("dirLight2.specular", { 1.0f, 1.0f, 1.0f });

		ourShader.setVec3("dirLight.direction", { -0.5f, -1.0f, -0.3f });
		ourShader.setVec3("dirLight.ambient", { 0.15f, 0.15f, 0.15f, });
		ourShader.setVec3("dirLight.diffuse", { 0.5f, 0.5f, 0.5f });
		ourShader.setVec3("dirLight.specular", { 1.0f, 1.0f, 1.0f });
		ourShader.setFloat("material.shininess", 30.0f);


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(m_window.get(), true);
		ImGui_ImplOpenGL3_Init("#version 130");


        while (!glfwWindowShouldClose(m_window.get()))
        {
			clear();
			showFPS();
			processInput();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();



			ourShader.use();

			std::vector<aiMatrix4x4> Transforms;
			ourModel.BoneTransform(glfwGetTime(), Transforms);

			for (int i = 0; i < Transforms.size(); i++) {
				ourShader.setMat4("gBones["s + std::to_string(i) + "]", Assimp2Glm(Transforms[i]));
			}

			ourShader.setVec3("viewPos", camera.Position);


			// view/projection transformations
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)800 / (float)600, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			ourShader.setMat4("projection", projection);
			ourShader.setMat4("view", view);


			
			/*ImGui::Begin("My First Tool");

			float x =0 , y = 0, z;

			ImGui::SliderFloat("floatX", &x, -3.0f, 3.0f);
			ImGui::SliderFloat("floatY", &y, -3.0f, 3.0f);
			ImGui::SliderFloat("floatZ", &z, -10.0f, 10.0f);

			ImGui::End();*/


			glm::mat4 model = glm::mat4(1.0f);

			model = glm::translate(model, glm::vec3(-0.03, -0.04, 2.8f));
			model = glm::scale(model, glm::vec3(0.002f, 0.002f, 0.002f));
			model = glm::rotate(model, glm::radians(180.f), glm::vec3(1.0f, 2.5f, 0.f));

			ourShader.setMat4("model", model);

			ourModel.Draw(ourShader);


			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(m_window.get());

            glfwPollEvents();
        }
	}

	void showFPS()
	{
		double currentTime = glfwGetTime();
		static double lastTime = 0;
		static double nbFrames = 0;
		double delta = currentTime - lastTime;

		nbFrames++;

		if (delta >= 1.0) { // If last cout was more than 1 sec ago
			double fps = double(nbFrames) / delta;

			glfwSetWindowTitle(m_window.get(), (std::string("FPS: ") + std::to_string(fps)).c_str());
			lastTime = currentTime;

			nbFrames = 0;
		}
	}

	void processInput()
	{
		static float deltaTime = 0.0f;	// Time between current frame and last frame
		static float lastFrame = 0.0f; // Time of last frame

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (glfwGetKey(m_window.get(), GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		if (glfwGetKey(m_window.get(), GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		if (glfwGetKey(m_window.get(), GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		if (glfwGetKey(m_window.get(), GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	}

	void clear()
	{
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
private:
	Window m_window;
};

RenderOpengl::RenderOpengl()
	: m_impl{ std::make_unique< RenderOpengl::Impl>() }
{

}

RenderOpengl::~RenderOpengl() = default;

void RenderOpengl::startRenderLoop()
{
	m_impl->startRenderLoop();
}
