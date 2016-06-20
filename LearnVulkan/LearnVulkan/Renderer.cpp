#include "Renderer.h"
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#endif

Renderer::Renderer()
{
	SetupDebug();
	InitGLFW();
	InitInstance();
	InitDebug();
	InitDevice();
	GLFWCreateSurface();
}


Renderer::~Renderer()
{
	GLFWDeleteSurface();
	DeleteDevice();
	DeleteDebug();
	DeleteInstance();
	DeleteGLFW();
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
	InstanceCreateInfo.pNext = &DebugReportInfo;

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
		std::cout << "\t" << i.layerName << " || " << i.description << "  " << std::endl;
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
	std::stringstream MessageStream;

	MessageStream << "VKDEBUG: ";
	switch (flags)
	{
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
		MessageStream << "[INFO] ";
		break;

	case  VK_DEBUG_REPORT_WARNING_BIT_EXT:
		MessageStream << "[WARNING] ";
		break;

	case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
		MessageStream << "[PERFORMANCE_WARN] ";
		break;
	case VK_DEBUG_REPORT_ERROR_BIT_EXT:
		MessageStream << "[ERROR] ";
		break;
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
		MessageStream << "[REPORT] ";
		break;
	case VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT:
		MessageStream << "[REPORT_FLAG] ";
		break;
	}
	MessageStream << pLayerPrefix << " | " << pMsg << " ";
	MessageStream << std::endl;
	std::cout << MessageStream.str();

	if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
#ifdef _WIN32
		MessageBox(NULL, (LPCWSTR)MessageStream.str().c_str() , L"Vulkan ERROR", 0);
#endif
	}

	return false;
}



void Renderer::SetupDebug()
{
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
	
	fvkCreateDebugReportCallbackEXT(Instance, &DebugReportInfo, nullptr, &DebugReport);
}

void Renderer::DeleteDebug()
{
	fvkDestroyDebugReportCallbackEXT(Instance, DebugReport, nullptr);
}

void Renderer::InitGLFW()
{
	glfwInit();

	// check for vulkan support
	if (GLFW_FALSE == glfwVulkanSupported())
	{
		// not supported
		glfwTerminate();
		std::exit(-1);
	}

	uint32_t instance_extension_count = 0;
	const char ** instance_extensions_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	for (uint32_t i = 0; i < instance_extension_count; ++i) {
		// Push back required instance extensions as well
		InstanceExtensions.push_back(instance_extensions_buffer[i]);
	}
}

void Renderer::DeleteGLFW()
{
	glfwTerminate();
}

void Renderer::GLFWCreateSurface()
{
	int width = 800;
	int height = 600;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// This tells GLFW to not create an OpenGL context with the window
	window = glfwCreateWindow(width, height,"Learn Vulkan", nullptr, nullptr);

	// make sure we indeed get the surface size we want.
	glfwGetFramebufferSize(window, &width, &height);


	VkResult ret = glfwCreateWindowSurface(Instance, window, nullptr, &surface);
	if (VK_SUCCESS != ret) {
		// couldn't create surface, exit
		glfwTerminate();
		std::exit(-1);
	}
}

void Renderer::GLFWDeleteSurface()
{
	vkDestroySurfaceKHR(Instance, surface, nullptr);
	glfwDestroyWindow(window);
}
