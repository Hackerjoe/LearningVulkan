#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <cstdlib>
#include <GLFW\glfw3.h>

class Renderer
{
public:
	Renderer();
	~Renderer();

	void InitInstance();
	void DeleteInstance();

	void InitDevice();
	void DeleteDevice();

	void SetupDebug();
	void InitDebug();
	void DeleteDebug();

	void InitGLFW();
	void DeleteGLFW();
	
	void GLFWCreateSurface();
	void GLFWDeleteSurface();
	
	void InitSwapchain();
	void DeleteSwapchain();

	void InitSwapImages();
	void DeleteSwapImages();

	void InitCommandPool();
	void DeleteCommandPool();

	void InitCommandBuffer();
	void DeleteCommandBuffer();
	void BeginCommandBuffer();
	void EndCommandBuffer();

	void CreateDepthBuffer();

	// VkInstance is where everything in Vulkan happens.
	VkInstance Instance = nullptr;
	// VkDevice handles the GPU and its queues.
	VkDevice Device = nullptr;
	// VkPhysicalDevice is the GPU.
	VkPhysicalDevice PhysicalDevice = nullptr;

	VkQueue Queue = nullptr;

	VkSurfaceKHR Surface = nullptr;

	VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};

	VkSurfaceFormatKHR SurfaceFormat = {};

	uint32_t SurfaceSizeX = 512;
	uint32_t SurfaceSizeY = 512;

	VkSwapchainKHR Swapchain = nullptr;
	uint32_t SwapchainImageCount = 2;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	GLFWwindow* window;
	VkDebugReportCallbackCreateInfoEXT DebugReportInfo = {};
	VkDebugReportCallbackEXT DebugReport = nullptr;

	uint32_t GraphicsFamilyIndex = 0;

	//Command Pool 
	VkCommandPool CommandPool = nullptr;

	//
	VkCommandBuffer CommandBuffer = nullptr;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
};