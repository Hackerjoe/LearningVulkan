#include "Renderer.h"
#include <vector>
#include <iostream>


Renderer::Renderer()
{
	InitInstance();
	InitDevice();
}


Renderer::~Renderer()
{
	DeleteDevice();
	DeleteInstance();
}

bool Renderer::InitInstance()
{
	// Welcome to Vulkan descriptor galore!
	VkApplicationInfo ApplicationInfo{};
	ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	ApplicationInfo.pApplicationName = "Learning Vulkan";

	VkInstanceCreateInfo InstanceCreateInfo{};
	InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;

	auto error = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);

	if (error != VK_SUCCESS)
		return false;

	return true;
}

void Renderer::DeleteInstance()
{
	vkDestroyInstance(Instance, nullptr);
	Instance = nullptr;
}

bool Renderer::InitDevice()
{
	uint32_t PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());

	PhysicalDevice = PhysicalDevices[0];


	uint32_t PhysicalDeviceQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &PhysicalDeviceQueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilyPropertiesList(PhysicalDeviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &PhysicalDeviceQueueFamilyCount, QueueFamilyPropertiesList.data());

	bool bFoundGraphicsFamily = false;
	for (uint32_t i = 0; i < PhysicalDeviceQueueFamilyCount; i++)
	{
		if (QueueFamilyPropertiesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			GraphicsFamilyIndex = i;
			bFoundGraphicsFamily = true;
		}
	}

	if (bFoundGraphicsFamily == false)
		return false;

	//
	uint32_t LayerCount = 0;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
	std::vector<VkLayerProperties> LayerProperties(LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.data());
	std::cout << "[Supported Layer Properties]" << std::endl;
	for (auto &i : LayerProperties)
	{
		std::cout << "  " << i.layerName << " || " << i.description << "  " << std::endl;
	}
	std::cout << "[END]" << std::endl;

	float QueuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo DeviceQueueCreateInfo{};
	DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	DeviceQueueCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

	VkDeviceCreateInfo DeviceCreateInfo{};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;

	auto error = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device);

	if (error != VK_SUCCESS)
		return false;

	return true;
}

void Renderer::DeleteDevice()
{
	vkDestroyDevice(Device, nullptr);
	Device = nullptr;
}