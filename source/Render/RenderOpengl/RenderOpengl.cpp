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

using namespace std::literals;

class RenderOpengl::Impl
{
public:
	struct MeshRenderData
	{
		unsigned int VAO = 0, VBO = 0, EBO = 0;
	};

	struct OpenglModel
	{
		std::unique_ptr<VulkanRender::Model> model;
		std::vector<MeshRenderData> meshRenderData;

		std::map<VulkanRender::Texture::Type, int> textures;

		glm::vec3 position{};
		glm::vec3 scale{};
	};
public:
	Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) };

	std::vector<OpenglModel> m_models;

	int m_modelsMeshCount = 0;
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

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 8);
		m_window = glfwCreateWindow(800, 600, "My Title", NULL, NULL);
		if (!m_window)
			throw std::runtime_error{ "glfwCreateWindow has failed" };

		glfwMakeContextCurrent(m_window);

		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwSwapInterval(0);

		glfwSetWindowUserPointer(m_window, this);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
			throw std::runtime_error{ "gladLoadGLLoader has failed" };

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

			_this->camera.ProcessMouseMovement(xoffset, yoffset);
		});
	}

	std::vector<OpenglModel> prepareModels(std::vector<ModelInfo> modelInfos)
	{
		std::vector<OpenglModel> models;

		for (auto& modelInfo : modelInfos)
		{
			OpenglModel model1;

			model1.model = std::make_unique<VulkanRender::Model>(modelInfo.modelPath,
				VulkanRender::Model::Textures{
					{modelInfo.texturePath, VulkanRender::Texture::Type::diffuse },
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

	MeshRenderData createMeshRenderData(VulkanRender::Mesh& mesh)
	{
		MeshRenderData meshRenderData;
		glGenVertexArrays(1, &meshRenderData.VAO);
		glGenBuffers(1, &meshRenderData.VBO);
		glGenBuffers(1, &meshRenderData.EBO);

		glBindVertexArray(meshRenderData.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, meshRenderData.VBO);

		glBufferData(GL_ARRAY_BUFFER, mesh.m_vertices.size() * sizeof(VulkanRender::Vertex), &mesh.m_vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshRenderData.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_indices.size() * sizeof(unsigned int),
			&mesh.m_indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VulkanRender::Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VulkanRender::Vertex), (void*)offsetof(VulkanRender::Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VulkanRender::Vertex), (void*)offsetof(VulkanRender::Vertex, TexCoords));

		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(VulkanRender::Vertex), (void*)offsetof(VulkanRender::Vertex, BoneIDs));

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, sizeof(VulkanRender::Vertex), (void*)(offsetof(VulkanRender::Vertex, BoneIDs) + 4 * sizeof(std::uint32_t)));

		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VulkanRender::Vertex), (void*)offsetof(VulkanRender::Vertex, Weights));

		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(VulkanRender::Vertex), (void*)(offsetof(VulkanRender::Vertex, Weights) + 4 * sizeof(float)));

		glBindVertexArray(meshRenderData.VAO);
		glDrawElements(GL_TRIANGLES, mesh.m_indices.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		return meshRenderData;
	}

	int createTextureImage(const std::string& path)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;

		unsigned char* data = VulkanRender::Model::loadTexture(path.c_str(), width, height);

		if (!data)
			throw std::runtime_error{ "Texture failed to load at path: "s + path };

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return textureID;
	}

	void startRenderLoop(std::vector<ModelInfo> modelInfos)
	{		
		init(std::move(modelInfos));

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		Shader ourShader(s_shader_v, s_shader_f);

		ourShader.use();
		ourShader.setInt("material.texture_diffuse1", 0);
		//ourShader.setInt("material.texture_specular1", 1);

        while (!glfwWindowShouldClose(m_window))
        {
			glClearColor(1.f, 1.f, 1.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			showFPS();
			processInput();


			ourShader.use();

			auto currentTime = glfwGetTime();

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)800 / (float)600, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();

			for (auto& model : m_models)
			{
				glm::mat4 modelMat = glm::mat4(1.0f);
				modelMat = glm::translate(modelMat, model.position);
				modelMat = glm::scale(modelMat, model.scale);
				//model = glm::rotate(model, glm::radians(180.f), glm::vec3(1.0f, 2.5f, 0.f));

				ourShader.setMat4("model", modelMat);
				ourShader.setMat4("PVM", projection * view * modelMat);

				std::vector<aiMatrix4x4> Transforms;
				model.model->BoneTransform(currentTime, Transforms);

				for (int i = 0; i < Transforms.size(); i++) {
					ourShader.setMat4("gBones["s + std::to_string(i) + "]", VulkanRender::Assimp2Glm(Transforms[i]));
				}

				auto findIt = model.textures.find(VulkanRender::Texture::Type::diffuse);
				if (findIt == model.textures.end())
					throw std::runtime_error{ "Can't find diffuse texture" };

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, findIt->second);

				findIt = model.textures.find(VulkanRender::Texture::Type::specular);
				if (findIt != model.textures.end())
				{
					glActiveTexture(GL_TEXTURE0 + 1);
					glBindTexture(GL_TEXTURE_2D, findIt->second);
				}
				else
				{
					glActiveTexture(GL_TEXTURE0 + 1);
					glBindTexture(GL_TEXTURE_2D, model.textures.find(VulkanRender::Texture::Type::diffuse)->second);
				}

				for (int i = 0; i < model.model->meshes.size(); ++i)
				{
					glBindVertexArray(model.meshRenderData[i].VAO);
					glDrawElements(GL_TRIANGLES, model.model->meshes[i].m_indices.size(), GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);
				}
			}

			glfwSwapBuffers(m_window);
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
	}

	void clear()
	{
		glClearColor(1.f, 1.f, 1.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
private:
	GLFWwindow* m_window;
};

RenderOpengl::RenderOpengl()
	: m_impl{ std::make_unique< RenderOpengl::Impl>() }
{

}

RenderOpengl::~RenderOpengl() = default;

void RenderOpengl::startRenderLoop(std::vector<ModelInfo> modelInfos)
{
	m_impl->startRenderLoop(std::move(modelInfos));
}
