#include "renderVulkan.h"
#include <iostream>
#include <vector>

#include "glad/vulkan.h"

#include "GLFW/glfw3.h"

static GLADapiproc glad_vulkan_callback(const char* name, void* user)
{
	return glfwGetInstanceProcAddress((VkInstance)user, name);
}

static void errorCcallback(int error, const char* description)
{
	std::cerr << "glfwError #" << error << " ; desc: " << description << std::endl;
}

class RenderVulkan::Impl
{
public:
	GLFWwindow* m_window = nullptr;
	VkInstance m_instance = nullptr;

	Impl()
	{
		if (!glfwInit())
			throw std::runtime_error{ "glfwInit has failed" };

		if (!glfwVulkanSupported())
			throw std::runtime_error{ "GLFW failed to find the Vulkan loader.\nExiting ...\n" };

		glfwSetErrorCallback(errorCcallback);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		
		GLFWwindow* window = glfwCreateWindow(800, 600, "Window Title", NULL, NULL);
		
		int glad_vk_version = gladLoaderLoadVulkan(NULL, NULL, NULL);

		createInstance();
	}

	void createInstance()
	{
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

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = nullptr;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions{ extensionCount };
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());




		if(vkCreateInstance(&createInfo, nullptr, &m_instance))
			throw std::runtime_error("failed to create instance!");


	}

	~Impl()
	{
		vkDestroyInstance(m_instance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void startRenderLoop()
	{

	}
};

RenderVulkan::RenderVulkan()
	: m_impl{ std::make_unique<RenderVulkan::Impl>() }
{

}

RenderVulkan::~RenderVulkan() = default;

void RenderVulkan::startRenderLoop()
{
	m_impl->startRenderLoop();
}
