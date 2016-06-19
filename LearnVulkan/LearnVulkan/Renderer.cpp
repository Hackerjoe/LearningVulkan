#include "Renderer.h"
#include <iostream>


Renderer::Renderer()
{
	SetupDebug();
	InitInstance();
	InitDebug();
	InitDevice();
}


Renderer::~Renderer()
{
	DeleteDevice();
	DeleteDebug();
	DeleteInstance();
}

void Renderer::InitInstance()
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
	InstanceCreateInfo.enabledLayerCount = InstanceLayers.size();
	InstanceCreateInfo.ppEnabledLayerNames = InstanceLayers.data();
	InstanceCreateInfo.enabledExtensionCount = InstanceExtensions.size();
	InstanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

	auto error = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);

	if (error != VK_SUCCESS)
		std::exit(-1); // Could not create instance.

}

void Renderer::DeleteInstance()
{
	vkDestroyInstance(Instance, nullptr);
	Instance = nullptr;
}

void Renderer::InitDevice()
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
		std::exit(-1); // Could not find graphics family.

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
	DeviceCreateInfo.enabledExtensionCount = DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	auto error = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device);

	if (error != VK_SUCCESS)
		std::exit(-1); // Could not create device.

}

void Renderer::DeleteDevice()
{
	vkDestroyDevice(Device, nullptr);
	Device = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugReportCallBack(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	size_t location,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* pUserData)
{
	std::cout << pMsg << std::endl;
	return false;
}



void Renderer::SetupDebug()
{
	InstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::InitDebug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugReportCallbackEXT");

	if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr)
	{
		// Could not fetch function pointers.
		std::exit(-1);
	}
	VkDebugReportCallbackCreateInfoEXT DebugReportInfo{};
	DebugReportInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	DebugReportInfo.flags = 
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT |
		0;
	DebugReportInfo.pfnCallback = VulkanDebugReportCallBack;
	fvkCreateDebugReportCallbackEXT(Instance, &DebugReportInfo, nullptr, &DebugReport);
}

void Renderer::DeleteDebug()
{
	fvkDestroyDebugReportCallbackEXT(Instance, DebugReport, nullptr);
}
