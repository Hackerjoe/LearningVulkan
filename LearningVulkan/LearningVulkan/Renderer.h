#pragma once

#include <vulkan\vulkan.h>

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool InitInstance();
	void DeleteInstance();

	bool InitDevice();
	void DeleteDevice();

	// VkInstance is where everything in Vulkan happens.
	VkInstance			Instance			= nullptr;
	// VkDevice handles the GPU and its queues.
	VkDevice			Device				= nullptr;
	// VkPhysicalDevice is the GPU.
	VkPhysicalDevice	PhysicalDevice		= nullptr;

	uint32_t			GraphicsFamilyIndex = 0;
};

