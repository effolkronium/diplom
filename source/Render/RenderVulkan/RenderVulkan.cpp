#include "RenderVulkan.h"

#include "glad/vulkan.h"
#include "GLFW/glfw3.h"

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stb_image.h>

#include "ShadersGen/Shaders.h"

#include "Utils.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>
#include <vector>
#include <set>
#include <array>
#include <algorithm>
#include <optional>
#include <cassert>
#include <memory>
#include <functional>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;
using namespace std::literals;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr uint32_t g_WIDTH = 1280;
constexpr uint32_t g_HEIGHT = 720;

const std::array<const char*, 1> g_validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::array<const char*, 1> g_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "VULKAN: " << pCallbackData->pMessage << std::endl << std::endl;

	return VK_FALSE;
}

static GLADapiproc glad_vulkan_callback(const char* name, void* user)
{
	return glfwGetInstanceProcAddress((VkInstance)user, name);
}

static void errorCcallback(int error, const char* description)
{
	std::cerr << "glfwError #" << error << " ; desc: " << description << std::endl;
}

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

class RenderVulkan::Impl
{
public:
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RenderCommon::Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RenderCommon::Vertex, Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RenderCommon::Vertex, Normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RenderCommon::Vertex, TexCoords);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[3].offset = offsetof(RenderCommon::Vertex, BoneIDs);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[4].offset = offsetof(RenderCommon::Vertex, BoneIDs) + sizeof(RenderCommon::Vertex::BoneIDs) / 2;

		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(RenderCommon::Vertex, Weights);

		attributeDescriptions[6].binding = 0;
		attributeDescriptions[6].location = 6;
		attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[6].offset = offsetof(RenderCommon::Vertex, Weights) + sizeof(RenderCommon::Vertex::Weights) / 2;

		return attributeDescriptions;
	}

	struct UniformBufferObject {
		constexpr static inline size_t MaxBoneTransforms = 100;
		alignas(16) glm::mat4 BoneTransform[MaxBoneTransforms];
		alignas(16) glm::vec3 viewPos;
	};

	struct PushConstantBufferObject {
		alignas(16) glm::mat4 PVM;
		alignas(16) glm::mat4 model;
	};
public:
	std::function<void(VkShaderModule)> m_shaderModuleDeleter =
		[this](VkShaderModule smodule) {
		vkDestroyShaderModule(m_device, smodule, nullptr);
	};

	std::function<void(VkBuffer)> m_bufferDeleter = [this](VkBuffer buffer) {
		if (buffer)
			vkDestroyBuffer(m_device, buffer, nullptr);
	};

	std::function<void(VkDeviceMemory)> m_deviceMemoryDeleter = [this](VkDeviceMemory deviceMemory) {
		if (deviceMemory)
			vkFreeMemory(m_device, deviceMemory, nullptr);
	};

	std::function<void(VkImage)> m_imageDeleter = [this](VkImage image) {
		if (image)
			vkDestroyImage(m_device, image, nullptr);
	};

	std::function<void(VkImageView)> m_imageViewDeleter = [this](VkImageView imageView) {
		if (imageView)
			vkDestroyImageView(m_device, imageView, nullptr);
	};

	std::function<void(VkSampler)> m_samplerDeleter = [this](VkSampler sampler) {
		if (sampler)
			vkDestroySampler(m_device, sampler, nullptr);
	};

	std::function<void(VkCommandPool)> m_commandPoolDeleter = [this](VkCommandPool commandPool) {
		if (commandPool)
			vkDestroyCommandPool(m_device, commandPool, nullptr);
	};

	using CommandBufferDeleter = std::function<void(VkCommandBuffer)>;
	
	using unique_ptr_shared_module = std::unique_ptr<std::remove_pointer_t<VkShaderModule>, decltype(m_shaderModuleDeleter)>;
	using unique_ptr_buffer = std::unique_ptr< std::remove_pointer_t<VkBuffer>, decltype(m_bufferDeleter)>;
	using unique_ptr_device_memory = std::unique_ptr< std::remove_pointer_t<VkDeviceMemory>, decltype(m_deviceMemoryDeleter)>;
	using unique_ptr_image = std::unique_ptr< std::remove_pointer_t<VkImage>, decltype(m_imageDeleter)>;
	using unique_ptr_image_view = std::unique_ptr< std::remove_pointer_t<VkImageView>, decltype(m_imageViewDeleter)>;
	using unique_ptr_sampler = std::unique_ptr< std::remove_pointer_t<VkSampler>, decltype(m_samplerDeleter)>;
	using unique_ptr_command_pool = std::unique_ptr< std::remove_pointer_t<VkCommandPool>, decltype(m_commandPoolDeleter)>;
	using unique_ptr_command_buffer = std::unique_ptr< std::remove_pointer_t<VkCommandBuffer>, CommandBufferDeleter>;
public:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct MeshVertexBuffer
	{
		MeshVertexBuffer(RenderVulkan::Impl* _this) : 
			m_vertexBuffer{ nullptr, _this->m_bufferDeleter },
			m_vertexBufferMemory{ nullptr, _this->m_deviceMemoryDeleter }
		{}

		unique_ptr_buffer m_vertexBuffer;
		unique_ptr_device_memory m_vertexBufferMemory;
	};

	struct MeshTextureImage
	{
		MeshTextureImage(RenderVulkan::Impl* _this) :
			textureImage{ nullptr, _this->m_imageDeleter },
			textureImageMemory{ nullptr, _this->m_deviceMemoryDeleter },
			textureImageView{ nullptr, _this->m_imageViewDeleter },
			textureSampler{ nullptr, _this->m_samplerDeleter }
		{}

		uint32_t mipLevels = 0;
		unique_ptr_image textureImage;
		unique_ptr_device_memory textureImageMemory;
		unique_ptr_image_view textureImageView;
		unique_ptr_sampler textureSampler;
	};

	struct MeshUniformBuffer
	{
		MeshUniformBuffer(RenderVulkan::Impl* _this) :
			uniformBuffer{ nullptr, _this->m_bufferDeleter },
			uniformBuffersMemory{ nullptr, _this->m_deviceMemoryDeleter }
		{}

		unique_ptr_buffer uniformBuffer;
		unique_ptr_device_memory uniformBuffersMemory;
		void* uniformBufferMemoryMapping;
	};

	struct VulkanModel
	{
		std::unique_ptr<RenderCommon::Model> model;

		std::map<RenderCommon::Texture::Type, MeshTextureImage*> meshTextureImages;
		std::vector<MeshVertexBuffer> meshVertexBuffers;
		std::vector<MeshUniformBuffer> meshUniformBuffers;
		std::vector<VkDescriptorSet> meshDescriptorSet;

		glm::vec3 position{};
		glm::vec3 scale{};

		std::vector<PushConstantBufferObject> pushConstant{};
		std::vector<UniformBufferObject> uniformBuffer{};

		ModelInfo info{};
	};

	struct ThreadData {
		ThreadData(RenderVulkan::Impl* self) :
			m_this{ self },
			commandPool{ nullptr, self->m_commandPoolDeleter }
		{		
		}

		ThreadData(ThreadData&&) = default;

		~ThreadData()
		{
			commandBuffers.clear();
		}
	
		RenderVulkan::Impl* m_this;
		unique_ptr_command_pool commandPool;

		std::vector<std::vector<unique_ptr_command_buffer>> commandBuffers;
		int usedCommandBuffers = 0;
	};
public:
	GLFWwindow* m_window = nullptr;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;

	std::vector<VkImageView> m_swapChainImageViews;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
	VkPipeline m_graphicsPipelineSimple = VK_NULL_HANDLE;

	std::vector<VkFramebuffer> m_swapChainFramebuffers;

	VkCommandPool m_commandPool = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;

	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;

	VkImage m_depthImage = VK_NULL_HANDLE;
	VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;

	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkImage colorImage = VK_NULL_HANDLE;
	VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
	VkImageView colorImageView = VK_NULL_HANDLE;
public:
	bool m_framebufferResized = false;
	int m_framebufferWidth = g_WIDTH;
	int m_framebufferHeight = g_HEIGHT;

	Camera camera{ glm::vec3(8.207467, 2.819616, 18.021290) };

	double m_deltaTime{};
	double m_currentTime{};
	double m_lastTime{};

	int m_MAX_FRAMES_IN_FLIGHT = 2;
	size_t m_currentFrame = 0;

	std::vector<VulkanModel> m_models;
	uint32_t m_modelsMeshCount = 0;

	int m_coreNumber = std::thread::hardware_concurrency();
	utils::ThreadPool m_threadPool{ m_coreNumber };
	std::vector<ThreadData> m_threadData;

	std::map<std::string, MeshTextureImage> m_imagesCache;
public:
	void cleanupSwapChain() {
		if (m_depthImageView)
		{
			vkDestroyImageView(m_device, m_depthImageView, nullptr);
			m_depthImageView = VK_NULL_HANDLE;
		}

		if (m_depthImage)
		{
			vkDestroyImage(m_device, m_depthImage, nullptr);
			m_depthImage = VK_NULL_HANDLE;
		}

		if (m_depthImageMemory)
		{
			vkFreeMemory(m_device, m_depthImageMemory, nullptr);
			m_depthImageMemory = VK_NULL_HANDLE;
		}

		if (colorImageView)
		{
			vkDestroyImageView(m_device, colorImageView, nullptr);
			colorImageView = VK_NULL_HANDLE;
		}

		if (colorImage)
		{
			vkDestroyImage(m_device, colorImage, nullptr);
			colorImage = VK_NULL_HANDLE;
		}

		if (colorImageMemory)
		{
			vkFreeMemory(m_device, colorImageMemory, nullptr);
			colorImageMemory = VK_NULL_HANDLE;
		}

		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		m_swapChainFramebuffers.clear();

		m_threadData.clear();

		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
		
		if (m_graphicsPipeline)
		{
			vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
			m_graphicsPipeline = VK_NULL_HANDLE;
		}

		if (m_pipelineLayout)
		{
			vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
			m_pipelineLayout = VK_NULL_HANDLE;
		}

		if (m_renderPass)
		{
			vkDestroyRenderPass(m_device, m_renderPass, nullptr);
			m_renderPass = VK_NULL_HANDLE;
		}

		for (auto imageView : m_swapChainImageViews)
			vkDestroyImageView(m_device, imageView, nullptr);
		m_swapChainImageViews.clear();

		if (m_swapChain)
		{
			vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
			m_swapChain = VK_NULL_HANDLE;
		}
	}

	~Impl()
	{
		cleanupSwapChain();

		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
			m_descriptorPool = VK_NULL_HANDLE;
		}

		for (auto& model : m_models)
		{
			model.meshTextureImages.clear();
			model.meshVertexBuffers.clear();
			model.meshUniformBuffers.clear();
		}

		m_imagesCache.clear();

		if (m_descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
			m_descriptorSetLayout = VK_NULL_HANDLE;
		}

		for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
		}
		m_renderFinishedSemaphores.clear();
		m_imageAvailableSemaphores.clear();
		m_inFlightFences.clear();

		if (m_commandPool)
		{
			vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
		}

		if (m_device)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}

		if (m_surface)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}

		if constexpr (enableValidationLayers)
		{
			vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
			m_debugMessenger = VK_NULL_HANDLE;
		}

		if (m_instance)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}

		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}

		//glfwTerminate();
	}

	Impl()
	{
	}

	void init(std::vector<ModelInfo> modelInfos)
	{
		auto models = prepareModels(std::move(modelInfos));
		createWindow();
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createColorResources();
		createDepthResources();
		createFramebuffers();
		createDescriptorPool();
		initThreadData();
		loadModels(std::move(models));
		createCommandBuffers();
		createSyncObjects();
	}

	void recreateSwapChain() {
		{
			glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
			std::this_thread::yield();
		} while (m_framebufferWidth == 0 || m_framebufferHeight == 0);

		vkDeviceWaitIdle(m_device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createColorResources();
		createDepthResources();
		createFramebuffers();
		createCommandBuffers();
	}

	std::vector<VulkanModel> prepareModels(std::vector<ModelInfo> modelInfos)
	{
		std::vector<VulkanModel> models;

		for (auto& modelInfo : modelInfos)
		{
			VulkanModel model1;

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

		for (VulkanModel& model : models)
		{
			for (size_t i = 0; i < model.model->meshes.size(); ++i)
			{
				++m_modelsMeshCount;
			}
		}

		return models;
	}

	void loadModels(std::vector<VulkanModel>&& models)
	{
		for (VulkanModel& model : models)
		{
			for (size_t i = 0; i < model.model->meshes.size(); ++i)
			{
				MeshVertexBuffer meshBuffer{ this };
				createVertexBuffer(model.model->meshes[i].m_vertices, model.model->meshes[i].m_indices, meshBuffer.m_vertexBuffer, meshBuffer.m_vertexBufferMemory);
				model.meshVertexBuffers.push_back(std::move(meshBuffer));

				for (auto& texture : model.model->meshes[i].m_textures)
				{
					auto findIt = model.meshTextureImages.find(texture.type);
					if (model.meshTextureImages.end() == findIt)
						model.meshTextureImages.emplace(texture.type, createTextureImage(texture.path));
				}

				++m_modelsMeshCount;
			}

			model.meshUniformBuffers = createMeshUniformBuffers();
			model.meshDescriptorSet = createDescriptorSets(model.meshUniformBuffers, model.meshTextureImages);
			model.pushConstant.resize(m_swapChainFramebuffers.size());
			model.uniformBuffer.resize(m_swapChainFramebuffers.size());

			m_models.emplace_back(std::move(model));
		}
	}

	void createWindow()
	{
		if (!glfwInit())
			throw std::runtime_error{ "glfwInit has failed" };

		if (!glfwVulkanSupported())
			throw std::runtime_error{ "GLFW failed to find the Vulkan loader.\nExiting ...\n" };

		glfwSetErrorCallback(errorCcallback);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(m_framebufferWidth, m_framebufferHeight, "Vulkan", NULL, NULL);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetWindowCenter(m_window);

		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int heigth) {
			auto _this = reinterpret_cast<RenderVulkan::Impl*>(glfwGetWindowUserPointer(window));
			_this->m_framebufferResized = true;
			_this->drawFrame();
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
			auto _this = reinterpret_cast<RenderVulkan::Impl*>(glfwGetWindowUserPointer(window));

			if (GLFW_CURSOR_NORMAL == glfwGetInputMode(window, GLFW_CURSOR))
				return;

			static float lastX = 400, lastY = 300;
			static bool firstMouse = true;
			if (firstMouse)
			{
				lastX = x;
				lastY = y;
				firstMouse = false;
			}

			float xoffset = x - lastX;
			float yoffset = lastY - y;
			lastX = x;
			lastY = y;

			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			else
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				_this->camera.ProcessMouseMovement(xoffset, yoffset);
		});
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = nullptr;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if constexpr (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void createInstance()
	{
		gladLoaderLoadVulkan(NULL, NULL, NULL);

		if constexpr (enableValidationLayers)
		{
			if (!checkValidationLayerSupport()) {
				throw std::runtime_error("validation layers requested, but not available!");
			}
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Diplom";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;


		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		if constexpr (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();

			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

			// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDebugUtilsMessageSeverityFlagBitsEXT.html
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			debugCreateInfo.pfnUserCallback = debugCallback;

			debugCreateInfo.pUserData = nullptr; // Optional

			createInfo.pNext = &debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		auto requiredExtensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = utils::intCast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions{ extensionCount };
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		if (vkCreateInstance(&createInfo, nullptr, &m_instance))
			throw std::runtime_error("if(vkCreateInstance(&createInfo, nullptr, &m_instance))");

		gladLoaderLoadVulkan(m_instance, NULL, NULL);

		if constexpr (enableValidationLayers)
		{
			if (vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger))
				throw std::runtime_error("if (vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger))");
		}
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount{};
		if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr))
			throw std::runtime_error("if(vkEnumerateInstanceLayerProperties(&layerCount, nullptr))");

		std::vector<VkLayerProperties> availableLayers(layerCount);

		if (vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()))
			throw std::runtime_error("if(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()))");

		return g_validationLayers.end() != std::find_if(begin(g_validationLayers), end(g_validationLayers), [&](auto& arg) {
			return availableLayers.end() != std::find_if(begin(availableLayers), end(availableLayers), [&](auto& prop) {
				return 0 == std::strcmp(prop.layerName, arg);
				});
			});
	}

	void createSurface() {
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface))
			throw std::runtime_error("failed to create window surface!");
	}

	VkSampleCountFlagBits getMaxUsableSampleCount() {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		int score = -1;
		for (const auto& device : devices) {
			int newScore = 0;
			QueueFamilyIndices indices{};
			if (isDeviceSuitable(device, newScore, indices)) {
				if (score < newScore)
				{
					score = newScore;
					m_physicalDevice = device;
					m_msaaSamples = getMaxUsableSampleCount();
				}
				break;
			}
		}

		if (m_physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("failed to find a suitable GPU!");

		gladLoaderLoadVulkan(m_instance, m_physicalDevice, NULL);
	}

	bool isDeviceSuitable(VkPhysicalDevice device, int& score, QueueFamilyIndices& indices) {
		if (!checkDeviceExtensionSupport(device))
			return false;

		bool swapChainAdequate = false;
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

		if (!swapChainAdequate)
			return false;

		indices = findQueueFamilies(device);

		if (!indices.graphicsFamily.has_value())
			return false;

		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures{};
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		score = 0;

		bool isOk = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		if (isOk) score += 10000;

		isOk = deviceFeatures.geometryShader;
		if (!isOk) return false;

		isOk = deviceFeatures.samplerAnisotropy;
		if (!isOk) return false;

		return true;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount{};
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(g_deviceExtensions.begin(), g_deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (indices.isComplete())
				break;

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			i++;
		}

		// TODO: Logic for preferring the same queue
		assert(indices.graphicsFamily.value() == indices.presentFamily.value());

		return indices;
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::vector<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
													  indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = utils::intCast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = utils::intCast<uint32_t>(g_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();


		// Deprecated for new vulkan implementations
		if constexpr (enableValidationLayers) {
			createInfo.enabledLayerCount = utils::intCast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device))
			throw std::runtime_error("failed to create logical device!");

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

		gladLoaderLoadVulkan(m_instance, m_physicalDevice, m_device);
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

		uint32_t formatCount{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount{};
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats.at(0);
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width{}, height{};
			glfwGetFramebufferSize(m_window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain))
			throw std::runtime_error("failed to create swap chain!");

		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

		//m_MAX_FRAMES_IN_FLIGHT = utils::intCast<int>(m_swapChainImages.size());
	}

	void createImageViews() {
		m_swapChainImageViews.resize(m_swapChainImages.size());

		for (uint32_t i = 0; i < m_swapChainImages.size(); i++) {
			m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapChainImageFormat;
		colorAttachment.samples = m_msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = m_msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = m_swapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass))
			throw std::runtime_error("failed to create render pass!");
	}

	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;

		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBindingSpecular{};
		samplerLayoutBindingSpecular.binding = 2;
		samplerLayoutBindingSpecular.descriptorCount = 1;
		samplerLayoutBindingSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBindingSpecular.pImmutableSamplers = nullptr;
		samplerLayoutBindingSpecular.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, samplerLayoutBindingSpecular };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = utils::intCast<uint32_t>(bindings.size());;
		layoutInfo.pBindings = bindings.data();;

		if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout))
			throw std::runtime_error("failed to create descriptor set layout!");
	}

	void createGraphicsPipeline() {
		unique_ptr_shared_module vertShaderModule{ createShaderModule(s_shader_vert), m_shaderModuleDeleter };
		unique_ptr_shared_module fragShaderModule{ createShaderModule(s_shader_frag), m_shaderModuleDeleter };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule.get();
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule.get();
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


		unique_ptr_shared_module vertShaderModuleSimple{ createShaderModule(s_shader_simple_vert), m_shaderModuleDeleter };
		unique_ptr_shared_module fragShaderModuleSimple{ createShaderModule(s_shader_simple_frag), m_shaderModuleDeleter };

		VkPipelineShaderStageCreateInfo vertShaderStageInfoSimple{};
		vertShaderStageInfoSimple.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfoSimple.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfoSimple.module = vertShaderModuleSimple.get();
		vertShaderStageInfoSimple.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfoSimple{};
		fragShaderStageInfoSimple.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfoSimple.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfoSimple.module = fragShaderModuleSimple.get();
		fragShaderStageInfoSimple.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStagesSimple[] = { vertShaderStageInfoSimple, fragShaderStageInfoSimple };

		
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		auto bindingDescription = getBindingDescription();
		auto attributeDescriptions = getAttributeDescriptions();

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = utils::intCast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainExtent.width;
		viewport.height = (float)m_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = m_msaaSamples;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional


		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantBufferObject);

		pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

		if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout))
			throw std::runtime_error("failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline))
			throw std::runtime_error("failed to create graphics pipeline!");

		pipelineInfo.pStages = shaderStagesSimple;

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineSimple))
			throw std::runtime_error("failed to create graphics pipeline!");
	}

	template<size_t N>
	VkShaderModule createShaderModule(const std::array<unsigned char, N>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule{};
		if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule))
			throw std::runtime_error("failed to create shader module!");

		return shaderModule;
	}

	void createFramebuffers() {
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
			std::array<VkImageView, 3> attachments = {
				colorImageView,
				m_depthImageView,
				m_swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;
			//std::cout << framebufferInfo.height << "\n\n";
			if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]))
				throw std::runtime_error("failed to create framebuffer!");
		}
	}

	unique_ptr_command_pool createCommandPoolPtr() {
		unique_ptr_command_pool commandPoolPtr{ nullptr, m_commandPoolDeleter };
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		// TODO
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		VkCommandPool commandPool = VK_NULL_HANDLE;
		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool))
			throw std::runtime_error("failed to create command pool!");

		commandPoolPtr.reset(commandPool);

		return commandPoolPtr;
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		// TODO
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool))
			throw std::runtime_error("failed to create command pool!");
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props{};
			vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	void createColorResources() {
		VkFormat colorFormat = m_swapChainImageFormat;

		createImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, colorFormat,
					VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

		colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		createImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);

		m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	}

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
					 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = numSamples;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(m_device, image, imageMemory, 0);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		endSingleTimeCommands(commandBuffer);
	}

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}


		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	MeshTextureImage* createTextureImage(const fs::path& path ) {

		auto findIt = m_imagesCache.find(path.string());

		if(findIt != m_imagesCache.end())
		{
			return &findIt->second;
		}

		MeshTextureImage textureImage{ this };
		int texWidth{}, texHeight{}, texChannels{};

		stbi_uc* pixels = RenderCommon::Model::loadTexture(path.string(), texWidth, texHeight);
		textureImage.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;


		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data = nullptr;
		vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, utils::intCast<size_t>(imageSize));
		vkUnmapMemory(m_device, stagingBufferMemory);

		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		createImage(texWidth, texHeight, textureImage.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

		textureImage.textureImage.reset(image);
		textureImage.textureImageMemory.reset(imageMemory);

		transitionImageLayout(textureImage.textureImage.get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureImage.mipLevels);
		copyBufferToImage(stagingBuffer, textureImage.textureImage.get(), utils::intCast<uint32_t>(texWidth), utils::intCast<uint32_t>(texHeight));
		//transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		generateMipmaps(textureImage.textureImage.get(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, textureImage.mipLevels);

		// TODO guard
		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);

		textureImage.textureImageView.reset(createTextureImageView(textureImage.textureImage.get(), textureImage.mipLevels));
		textureImage.textureSampler.reset(createTextureSampler(textureImage.mipLevels));

		m_imagesCache.emplace(path.string(), std::move(textureImage));

		findIt = m_imagesCache.find(path.string());

		return &findIt->second;
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	VkImageView createTextureImageView(VkImage image, uint32_t mipLevels) {
		return createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}

	VkSampler createTextureSampler(uint32_t mipLevels) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		// TODO sync with opengl
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f; // Optional
		samplerInfo.maxLod = static_cast<float>(mipLevels);
		samplerInfo.mipLodBias = 0.0f; // Optional

		VkSampler textureSampler = VK_NULL_HANDLE;
		if (vkCreateSampler(m_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		return textureSampler;
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphicsQueue);

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	// TODO
	void createVertexBuffer(const std::vector<RenderCommon::Vertex>& vertices, const std::vector<uint32_t>& indices,
							unique_ptr_buffer& vertexBuffer, unique_ptr_device_memory&  vertexBufferMemory) {
		VkDeviceSize vertexSize = sizeof(vertices[0]) * vertices.size();
		VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();

		VkDeviceSize bufferSize = vertexSize + indexSize;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		unsigned char* data = nullptr;
		vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data));
		memcpy(data, vertices.data(), vertexSize);
		memcpy(data + vertexSize, indices.data(), indexSize);
		vkUnmapMemory(m_device, stagingBufferMemory);

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory bufMem = VK_NULL_HANDLE;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufMem);

		vertexBuffer.reset(buffer);
		vertexBufferMemory.reset(bufMem);

		copyBuffer(stagingBuffer, vertexBuffer.get(), bufferSize);

		// TODO guard
		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}

	void createDescriptorPool() {
		std::vector<VkDescriptorPoolSize> poolSizes{};
		poolSizes.resize(1000);
		for (int i = 0; i < 500; ++i)
		{
			poolSizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[i].descriptorCount = static_cast<uint32_t>(m_swapChainImages.size()) * 1000;
			poolSizes[++i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[i].descriptorCount = static_cast<uint32_t>(m_swapChainImages.size()) * 1000;
		}


		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(m_swapChainImages.size()) * m_modelsMeshCount * 1000;// *m_model;

		if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool))
			throw std::runtime_error("failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSet> createDescriptorSets(const std::vector<MeshUniformBuffer>& uniformBuffers, const std::map<RenderCommon::Texture::Type, MeshTextureImage*>& textures) {
		std::vector<VkDescriptorSetLayout> layouts(m_swapChainImages.size(), m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		std::vector<VkDescriptorSet> descriptorSets;
		descriptorSets.resize(m_swapChainImages.size());
		if (vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()))
			throw std::runtime_error("failed to allocate descriptor sets!");

		auto findIt = textures.find(RenderCommon::Texture::Type::diffuse);
		if (findIt == textures.end())
			throw std::runtime_error{ "Can't find diffuse texture" };

		auto& diffuseTexture = *findIt->second;
		const MeshTextureImage* specularTexture = nullptr;
		findIt = textures.find(RenderCommon::Texture::Type::specular);
		if (findIt != textures.end())
			specularTexture = findIt->second;

		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].uniformBuffer.get();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = diffuseTexture.textureImageView.get();
			imageInfo.sampler = diffuseTexture.textureSampler.get();

			VkDescriptorImageInfo imageInfoSpecular{};
			if (specularTexture)
			{
				imageInfoSpecular.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfoSpecular.imageView = specularTexture->textureImageView.get();
				imageInfoSpecular.sampler = specularTexture->textureSampler.get();
			}

			std::vector<VkWriteDescriptorSet> descriptorWrites{2};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			if (specularTexture)
			{
				descriptorWrites.push_back({});
				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[i];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &imageInfoSpecular;
			}
			else
			{
				descriptorWrites.push_back({});
				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[i];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &imageInfo;
			}

			vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return descriptorSets;
	}

	std::vector<MeshUniformBuffer> createMeshUniformBuffers() {
		std::vector<MeshUniformBuffer> uniformBuffers;

		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.reserve(m_swapChainImages.size());

		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				buffer, bufferMemory);

			uniformBuffers.emplace_back(this);
			uniformBuffers.back().uniformBuffer.reset(buffer);
			uniformBuffers.back().uniformBuffersMemory.reset(bufferMemory);
			vkMapMemory(m_device, uniformBuffers.back().uniformBuffersMemory.get(), 0, sizeof(UniformBufferObject), 0, &uniformBuffers.back().uniformBufferMemoryMapping);
		}

		return uniformBuffers;
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	unique_ptr_command_buffer createCommandBufferPtrSecondary(VkCommandPool commandPool, CommandBufferDeleter cmdBufferDeleter)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer))
			throw std::runtime_error("failed to allocate command buffers!");

		unique_ptr_command_buffer commadBufferPtr{ nullptr, cmdBufferDeleter };

		commadBufferPtr.reset(commandBuffer);

		return commadBufferPtr;
	}

	void createCommandBuffers()
	{
		m_commandBuffers.resize(m_swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = utils::intCast<uint32_t>(m_commandBuffers.size());

		if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()))
			throw std::runtime_error("failed to allocate command buffers!");
	}

	void createSyncObjects() {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		m_imageAvailableSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(m_MAX_FRAMES_IN_FLIGHT);
		m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) ||
				vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) ||
				vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i])) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void showFPS()
	{
		static double lastFpsUpdateTime = 0;
		static double nbFrames = 0;
		double delta = m_currentTime - lastFpsUpdateTime;

		nbFrames++;

		if (delta >= 1.0) { // If last fps update was more than 1 sec ago
			double fps = static_cast<double>(nbFrames) / delta;

			glfwSetWindowTitle(m_window, (std::string("VULAKN FPS: ") + std::to_string(fps)).c_str());
			lastFpsUpdateTime = m_currentTime;
			nbFrames = 0;
		}
	}

	void updateDeltaTime()
	{
		m_currentTime = glfwGetTime();
		m_deltaTime = m_currentTime - m_lastTime;
		m_lastTime = m_currentTime;
	}

	void processInput()
	{
		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::FORWARD, m_deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::BACKWARD, m_deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::LEFT, m_deltaTime);
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::RIGHT, m_deltaTime);

		if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void initThreadData()
	{
		for (int i = 0; i < m_coreNumber; ++i)
		{
			ThreadData threadData{ this };
			threadData.commandPool = createCommandPoolPtr();
			m_threadData.emplace_back(std::move(threadData));

			for (size_t j = 0; j < m_swapChainFramebuffers.size(); ++j)
			{
				m_threadData[i].commandBuffers.push_back({});
				
			}
		}
	}

	bool updateModelPushConstants(uint32_t currentImage, VulkanModel& vulkanModel)
	{
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(45.f), m_framebufferWidth / static_cast<float>(m_framebufferHeight), 0.1f, 200.f);

		proj[1][1] *= -1;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, vulkanModel.position);
		model = glm::scale(model, vulkanModel.scale);

		if(vulkanModel.info.simpleModel)
			model = glm::rotate(model, (float)m_currentTime, glm::vec3(0.5f, 1.0f, 0.0f));

		glm::mat4 PVM = proj * view * model;

		vulkanModel.pushConstant[currentImage].PVM = PVM;
		vulkanModel.pushConstant[currentImage].model = model;

		return true;
	}

	void updateUniformBuffer(uint32_t currentImage, VulkanModel& vulkanMode) {
		std::vector<aiMatrix4x4> boneTransforms;
		vulkanMode.model->BoneTransform(m_currentTime, boneTransforms);

		for (size_t i = 0; i < boneTransforms.size(); ++i)
			vulkanMode.uniformBuffer[currentImage].BoneTransform[i] = RenderCommon::Assimp2Glm(boneTransforms[i]);

		std::memcpy(vulkanMode.meshUniformBuffers[currentImage].uniformBufferMemoryMapping,
					vulkanMode.uniformBuffer[currentImage].BoneTransform,
					boneTransforms.size() * sizeof(boneTransforms[0]));

		std::memcpy((char*)vulkanMode.meshUniformBuffers[currentImage].uniformBufferMemoryMapping + offsetof(UniformBufferObject, viewPos),
					&camera.Position, sizeof(camera.Position));
	}

	void updateSecondaryCommandBuffers(uint32_t currentImage)
	{
		for (auto& threadData : m_threadData)
			threadData.usedCommandBuffers = 0;

		VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
		cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		cmdBufferInheritanceInfo.renderPass = m_renderPass;
		cmdBufferInheritanceInfo.framebuffer = m_swapChainFramebuffers[currentImage];
		
		std::vector<std::future<void>> futures;

		for (size_t i = 0; i < m_models.size(); ++i)
		{
			auto renderTask = m_threadPool.enqueue([&](VulkanModel* vulkanModel) {
				int threadId = m_threadPool.threadIndex();
				auto& threadData = m_threadData[threadId];
				auto& commandBuffers = threadData.commandBuffers[currentImage];

				++threadData.usedCommandBuffers;
				int currentCommandBufferIndex = threadData.usedCommandBuffers - 1;

				if (commandBuffers.size() < threadData.usedCommandBuffers)
				{
					commandBuffers.emplace_back(createCommandBufferPtrSecondary(threadData.commandPool.get(), [this, wtf = &m_threadData[threadId]](VkCommandBuffer buffer) {
						if (buffer && wtf->commandPool)
							vkFreeCommandBuffers(m_device, wtf->commandPool.get(), 1, &buffer);
					}));
				}

				auto& commandBuffer = commandBuffers[currentCommandBufferIndex];

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // Optional
				beginInfo.pInheritanceInfo = &cmdBufferInheritanceInfo; // Optional

				if (vkBeginCommandBuffer(commandBuffer.get(), &beginInfo))
					throw std::runtime_error("failed to begin recording command buffer!");

				if(vulkanModel->info.simpleModel)
					vkCmdBindPipeline(commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineSimple);
				else
					vkCmdBindPipeline(commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

				if (updateModelPushConstants(currentImage, *vulkanModel))
					vkCmdPushConstants(commandBuffer.get(), m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantBufferObject), &vulkanModel->pushConstant[currentImage]);

				updateUniformBuffer(currentImage, *vulkanModel);

				for (size_t j = 0; j < vulkanModel->model->meshes.size(); ++j)
				{
					VkBuffer vertexBuffers[] = { vulkanModel->meshVertexBuffers[j].m_vertexBuffer.get() };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer.get(), 0, 1, vertexBuffers, offsets);
					vkCmdBindIndexBuffer(commandBuffer.get(), vulkanModel->meshVertexBuffers[j].m_vertexBuffer.get(),
						sizeof(vulkanModel->model->meshes[j].m_vertices[0]) * vulkanModel->model->meshes[j].m_vertices.size(), VK_INDEX_TYPE_UINT32);

					vkCmdBindDescriptorSets(commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &vulkanModel->meshDescriptorSet[currentImage], 0, nullptr);

					vkCmdDrawIndexed(commandBuffer.get(), utils::intCast<uint32_t>(vulkanModel->model->meshes[j].m_indices.size()), 1, 0, 0, 0);
				}

				if (vkEndCommandBuffer(commandBuffer.get()))
					throw std::runtime_error("failed to record command buffer!");


				}, &m_models[i]);

			futures.emplace_back(std::move(renderTask));
		}

		for (auto& task : futures)
		{
			task.get();
		}
	}

	void updateCommandBuffers(uint32_t currentImage)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_commandBuffers[currentImage], &beginInfo))
			throw std::runtime_error("failed to begin recording command buffer!");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[currentImage];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		updateSecondaryCommandBuffers(currentImage);

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = VkClearColorValue{ 135 / 255.f, 206 / 255.f, 235 / 255.f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_commandBuffers[currentImage], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.reserve(m_models.size());

		for (auto& threadData : m_threadData)
			for(size_t i = 0; i < threadData.usedCommandBuffers; ++i)
				commandBuffers.push_back(threadData.commandBuffers[currentImage][i].get());

		vkCmdExecuteCommands(m_commandBuffers[currentImage], commandBuffers.size(), commandBuffers.data());

		vkCmdEndRenderPass(m_commandBuffers[currentImage]);

		if (vkEndCommandBuffer(m_commandBuffers[currentImage]))
			throw std::runtime_error("failed to record command buffer!");
	}

	// Rendering loop
	double startRenderLoop(std::vector<ModelInfo> modelInfos)
	{
		init(std::move(modelInfos));

		std::uint64_t frameCount = 0;
		auto startTime = glfwGetTime();
		while (!glfwWindowShouldClose(m_window)) {		
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				break;

			updateDeltaTime();
			processInput();
			showFPS();

			drawFrame();
			frameCount++;

			glfwPollEvents();

			auto endSeconds = glfwGetTime();
			auto renderSeconds = endSeconds - startTime;

			if (renderSeconds > 60)
				break;
		}

		auto endTime = glfwGetTime();
		auto renderTime = endTime - startTime;
		auto averageFps = frameCount / (double)renderTime;

		vkDeviceWaitIdle(m_device);

		return averageFps;
	}

	void drawFrame() {
		while (true)
		{
			uint32_t imageIndex{};
			VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				recreateSwapChain();
				continue;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				throw std::runtime_error("failed to acquire swap chain image!");
			}

			if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
				vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
			}

			m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

			updateCommandBuffers(imageIndex);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

			VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
			if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]))
				throw std::runtime_error("failed to submit draw command buffer!");

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;
			VkSwapchainKHR swapChains[] = { m_swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr; // Optional
			result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
				m_framebufferResized = false;
				recreateSwapChain();
				continue;
			}
			else if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}

			m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
			break;
		}
	}
};

RenderVulkan::RenderVulkan()
	: m_impl{ std::make_unique<RenderVulkan::Impl>() }
{

}

RenderVulkan::~RenderVulkan() = default;

double RenderVulkan::startRenderLoop(std::vector<ModelInfo> modelInfos)
{
	return m_impl->startRenderLoop(std::move(modelInfos));
}
