#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <cstdlib>

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
	

	// VkInstance is where everything in Vulkan happens.
	VkInstance Instance = nullptr;
	// VkDevice handles the GPU and its queues.
	VkDevice Device = nullptr;
	// VkPhysicalDevice is the GPU.
	VkPhysicalDevice PhysicalDevice = nullptr;

	VkDebugReportCallbackEXT DebugReport = nullptr;

	uint32_t GraphicsFamilyIndex = 0;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
};