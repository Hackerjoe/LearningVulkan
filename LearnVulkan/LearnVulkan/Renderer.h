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
	

	// VkInstance is where everything in Vulkan happens.
	VkInstance Instance = nullptr;
	// VkDevice handles the GPU and its queues.
	VkDevice Device = nullptr;
	// VkPhysicalDevice is the GPU.
	VkPhysicalDevice PhysicalDevice = nullptr;

	VkSurfaceKHR surface = nullptr;

	GLFWwindow* window;
	VkDebugReportCallbackCreateInfoEXT DebugReportInfo = {};
	VkDebugReportCallbackEXT DebugReport = nullptr;

	uint32_t GraphicsFamilyIndex = 0;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
};