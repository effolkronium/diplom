#include "RenderOpengl.h"

#include "glad/glad.h"
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
#include <Utils.h>

using namespace std::literals;

static void glfwSetWindowCenter(GLFWwindow* window) {
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

class RenderOpengl::Impl
{
public:
	struct MeshRenderData
	{
		unsigned int VAO = 0, VBO = 0, EBO = 0;
	};

	struct OpenglModel
	{
		std::unique_ptr<RenderCommon::Model> model;
		std::vector<MeshRenderData> meshRenderData;

		std::map<RenderCommon::Texture::Type, int> textures;

		glm::vec3 position{};
		glm::vec3 scale{};

		ModelInfo info{};
	};
public:
	Camera camera{ glm::vec3(8.207467, 2.819616, 18.021290) };

	std::vector<OpenglModel> m_models;

	int m_modelsMeshCount = 0;

	std::map<std::string, int> m_textureCache;
public:
	Impl()
	{
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

	void init(std::vector<ModelInfo> modelInfos)
	{
		initWindow();

		glEnable(GL_MULTISAMPLE);
		glEnable(GL_FRAMEBUFFER_SRGB);

		auto models = prepareModels(std::move(modelInfos));
		loadModels(std::move(models));
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
		m_window = glfwCreateWindow(1280, 720, "OpenGL", NULL, NULL);
		if (!m_window)
			throw std::runtime_error{ "glfwCreateWindow has failed" };

		glfwMakeContextCurrent(m_window);

		glfwSwapInterval(0);

		glfwSetWindowUserPointer(m_window, this);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
			throw std::runtime_error{ "gladLoadGLLoader has failed" };

		glfwSetWindowCenter(m_window);

		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int x, int y) {
			auto _this = reinterpret_cast<RenderOpengl::Impl*>(glfwGetWindowUserPointer(window));
				glViewport(0, 0, x, y);
				// render(); 
			});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
			auto _this = reinterpret_cast<RenderOpengl::Impl*>(glfwGetWindowUserPointer(window));
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

			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			else
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				_this->camera.ProcessMouseMovement(xoffset, yoffset);
		});
	}

	std::vector<OpenglModel> prepareModels(std::vector<ModelInfo> modelInfos)
	{
		std::vector<OpenglModel> models;

		for (auto& modelInfo : modelInfos)
		{
			OpenglModel model1;

			model1.info = modelInfo;

			model1.model = std::make_unique<RenderCommon::Model>(modelInfo.modelPath,
				RenderCommon::Model::Textures{
					{modelInfo.texturePath, RenderCommon::Texture::Type::diffuse },
				},
				modelInfo.animationNumber
			);

			model1.position = { modelInfo.posX, modelInfo.posY, modelInfo.posZ };
			model1.scale = { modelInfo.scaleX, modelInfo.scaleY, modelInfo.scaleZ };

			models.emplace_back(std::move(model1));
		}

		for (OpenglModel& model : models)
		{
			for (size_t i = 0; i < model.model->meshes.size(); ++i)
			{
				++m_modelsMeshCount;
			}
		}

		return models;
	}

	void loadModels(std::vector<OpenglModel> models)
	{
		for (OpenglModel& model : models)
		{
			for (size_t i = 0; i < model.model->meshes.size(); ++i)
			{
				model.meshRenderData.push_back(createMeshRenderData(model.model->meshes[i]));

				for (auto& texture : model.model->meshes[i].m_textures)
				{
					auto findIt = model.textures.find(texture.type);
					if (model.textures.end() == findIt)
						model.textures.emplace(texture.type, createTextureImage(texture.path));
				}
			}
		}

		m_models = std::move(models);
	}

	MeshRenderData createMeshRenderData(RenderCommon::Mesh& mesh)
	{
		MeshRenderData meshRenderData;
		glGenVertexArrays(1, &meshRenderData.VAO);
		glGenBuffers(1, &meshRenderData.VBO);
		glGenBuffers(1, &meshRenderData.EBO);

		glBindVertexArray(meshRenderData.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, meshRenderData.VBO);

		glBufferData(GL_ARRAY_BUFFER, mesh.m_vertices.size() * sizeof(RenderCommon::Vertex), &mesh.m_vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshRenderData.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_indices.size() * sizeof(unsigned int),
			&mesh.m_indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderCommon::Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderCommon::Vertex), (void*)offsetof(RenderCommon::Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderCommon::Vertex), (void*)offsetof(RenderCommon::Vertex, TexCoords));

		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(RenderCommon::Vertex), (void*)offsetof(RenderCommon::Vertex, BoneIDs));

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, sizeof(RenderCommon::Vertex), (void*)(offsetof(RenderCommon::Vertex, BoneIDs) + 4 * sizeof(std::uint32_t)));

		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(RenderCommon::Vertex), (void*)offsetof(RenderCommon::Vertex, Weights));

		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(RenderCommon::Vertex), (void*)(offsetof(RenderCommon::Vertex, Weights) + 4 * sizeof(float)));

		glBindVertexArray(meshRenderData.VAO);
		glDrawElements(GL_TRIANGLES, mesh.m_indices.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		return meshRenderData;
	}

	int createTextureImage(const std::string& path)
	{
		auto findIt = m_textureCache.find(path);

		if(findIt != m_textureCache.end())
		{
			return findIt->second;
		}

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;

		unsigned char* data = RenderCommon::Model::loadTexture(path.c_str(), width, height);

		if (!data)
			throw std::runtime_error{ "Texture failed to load at path: "s + path };

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		m_textureCache.emplace(path, textureID);

		return textureID;
	}

	double startRenderLoop(std::vector<ModelInfo> modelInfos)
	{		
		init(std::move(modelInfos));

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glClearColor(135 / 255.f, 206 / 255.f, 235 / 255.f, 1.0f);

		Shader ourShaderSimple(s_shader_v_simple, s_shader_f_simple);
		Shader ourShader(s_shader_v, s_shader_f);

		Shader* currentShader = &ourShader;

		ourShader.use();
		ourShader.setInt("material.texture_diffuse1", 0);
		//ourShader.setInt("material.texture_specular1", 1);

		auto startSeconds = glfwGetTime();
		std::uint64_t frameCount = 0;
        while (!glfwWindowShouldClose(m_window))
        {
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				break;

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			showFPS();
			processInput();

			auto currentTime = glfwGetTime();

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();

			for (auto& model : m_models)
			{
				if (model.info.simpleModel)
					currentShader = &ourShaderSimple;
				else
					currentShader = &ourShader;

				currentShader->use();

				glm::mat4 modelMat = glm::mat4(1.0f);
				modelMat = glm::translate(modelMat, model.position);
				modelMat = glm::scale(modelMat, model.scale);

				if (model.info.simpleModel)
					modelMat = glm::rotate(modelMat, (float)currentTime, glm::vec3(0.5f, 1.0f, 0.0f));

				currentShader->setMat4("model", modelMat);
				currentShader->setMat4("PVM", projection * view * modelMat);

				std::vector<aiMatrix4x4> Transforms;
				model.model->BoneTransform(currentTime, Transforms);

				
				if (!model.info.simpleModel)
				for (int i = 0; i < Transforms.size(); i++) {
					currentShader->setMat4("gBones["s + std::to_string(i) + "]", RenderCommon::Assimp2Glm(Transforms[i]));
				}

				auto findIt = model.textures.find(RenderCommon::Texture::Type::diffuse);
				if (findIt == model.textures.end())
					throw std::runtime_error{ "Can't find diffuse texture" };

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, findIt->second);

				findIt = model.textures.find(RenderCommon::Texture::Type::specular);
				if (findIt != model.textures.end())
				{
					glActiveTexture(GL_TEXTURE0 + 1);
					glBindTexture(GL_TEXTURE_2D, findIt->second);
				}
				else
				{
					glActiveTexture(GL_TEXTURE0 + 1);
					glBindTexture(GL_TEXTURE_2D, model.textures.find(RenderCommon::Texture::Type::diffuse)->second);
				}

				for (int i = 0; i < model.model->meshes.size(); ++i)
				{
					glBindVertexArray(model.meshRenderData[i].VAO);
					glDrawElements(GL_TRIANGLES, model.model->meshes[i].m_indices.size(), GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);
				}
			}

			glfwSwapBuffers(m_window);
			frameCount++;

			auto endSeconds = glfwGetTime();
			auto renderSeconds = endSeconds - startSeconds;

			if (renderSeconds > 60)
				break;

            glfwPollEvents();
        }
		
		auto endSeconds = glfwGetTime();
		auto renderSeconds = endSeconds - startSeconds;
		auto averageFps = frameCount / renderSeconds;

		glfwDestroyWindow(m_window);

		return averageFps;
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

			glfwSetWindowTitle(m_window, (std::string("OPENGL FPS: ") + std::to_string(fps)).c_str());
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

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);


		if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
private:
	GLFWwindow* m_window;
};

RenderOpengl::RenderOpengl()
	: m_impl{ std::make_unique< RenderOpengl::Impl>() }
{

}

RenderOpengl::~RenderOpengl() = default;

double RenderOpengl::startRenderLoop(std::vector<ModelInfo> modelInfos)
{
	return m_impl->startRenderLoop(std::move(modelInfos));
}
