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
	InitCommandPool();
	InitCommandBuffer();
	InitSwapchain();
	InitSwapImages();
}


Renderer::~Renderer()
{
	DeleteSwapImages();
	DeleteSwapchain();
	DeleteCommandBuffer();
	DeleteCommandPool();
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

	vkGetDeviceQueue(Device, GraphicsFamilyIndex, 0, &Queue);

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
		MessageBox(NULL, L"ERROR Check Log", L"Vulkan ERROR", 0);
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

	InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	uint32_t instance_extension_count = 0;
	const char ** instance_extensions_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	for (uint32_t i = 0; i < instance_extension_count; ++i) {
		// Push back required instance extensions as well
		InstanceExtensions.push_back(instance_extensions_buffer[i]);
	}

	DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::DeleteGLFW()
{
	glfwTerminate();
}

void Renderer::GLFWCreateSurface()
{
	int width = 512;
	int height = 512;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// This tells GLFW to not create an OpenGL context with the window
	window = glfwCreateWindow(width, height, "Learn Vulkan", nullptr, nullptr);

	// make sure we indeed get the surface size we want.
	glfwGetFramebufferSize(window, &width, &height);


	VkResult ret = glfwCreateWindowSurface(Instance, window, nullptr, &Surface);
	if (VK_SUCCESS != ret) {
		// couldn't create surface, exit
		glfwTerminate();
		std::exit(-1);
	}

	VkBool32 WSI_supported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GraphicsFamilyIndex, Surface, &WSI_supported);
	if (!WSI_supported) {
		std::exit(-1);
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);
	if (SurfaceCapabilities.currentExtent.width < UINT32_MAX) {
		SurfaceSizeX = SurfaceCapabilities.currentExtent.width;
		SurfaceSizeY = SurfaceCapabilities.currentExtent.height;
	}

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &format_count, nullptr);
	if (format_count == 0)
	{
		std::exit(-1);
	}
	std::vector<VkSurfaceFormatKHR> formats(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &format_count, formats.data());
	if (formats[0].format == VK_FORMAT_UNDEFINED) {
		SurfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		SurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else {
		SurfaceFormat = formats[0];
	}
}

void Renderer::GLFWDeleteSurface()
{
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	glfwDestroyWindow(window);
}

void Renderer::InitSwapchain()
{
	if (SwapchainImageCount < SurfaceCapabilities.minImageCount + 1)
		SwapchainImageCount = SurfaceCapabilities.minImageCount + 1;
	if (SurfaceCapabilities.maxImageCount > 0)
	{
		if (SwapchainImageCount > SurfaceCapabilities.maxImageCount)
			SwapchainImageCount = SurfaceCapabilities.maxImageCount;
	}

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModeList(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, presentModeList.data());
	for (auto m : presentModeList) {
		if (m == VK_PRESENT_MODE_MAILBOX_KHR) presentMode = m;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info{};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = Surface;
	swapchain_create_info.minImageCount = SwapchainImageCount;
	swapchain_create_info.imageFormat = SurfaceFormat.format;
	swapchain_create_info.imageColorSpace = SurfaceFormat.colorSpace;
	swapchain_create_info.imageExtent.width = SurfaceSizeX;
	swapchain_create_info.imageExtent.height = SurfaceSizeY;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = nullptr;
	swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = presentMode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

	auto error = vkCreateSwapchainKHR(Device, &swapchain_create_info, nullptr, &Swapchain);
	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}

	error = vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, nullptr);
	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}
}

void Renderer::DeleteSwapchain()
{
	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
}

void Renderer::InitSwapImages()
{
	SwapchainImages.resize(SwapchainImageCount);
	SwapchainImageViews.resize(SwapchainImageCount);

	auto error = vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, SwapchainImages.data());
	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}

	for (uint32_t i = 0; i < SwapchainImageCount; ++i) {
		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = SwapchainImages[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = SurfaceFormat.format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		vkCreateImageView(Device, &image_view_create_info, nullptr, &SwapchainImageViews[i]);
	}
}

void Renderer::DeleteSwapImages()
{
	for (auto view : SwapchainImageViews) {
		vkDestroyImageView(Device, view, nullptr);
	}
}

void Renderer::InitCommandPool()
{
	VkCommandPoolCreateInfo CmdPoolInfo = {};
	CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CmdPoolInfo.pNext = NULL;
	CmdPoolInfo.queueFamilyIndex = GraphicsFamilyIndex;
	CmdPoolInfo.flags = 0;

	auto error = vkCreateCommandPool(Device, &CmdPoolInfo, nullptr, &CommandPool);

	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}
}

void Renderer::DeleteCommandPool()
{
	vkDestroyCommandPool(Device, CommandPool, nullptr);
}

void Renderer::InitCommandBuffer()
{
	VkCommandBufferAllocateInfo CmdBufferInfo = {};
	CmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CmdBufferInfo.pNext = NULL;
	CmdBufferInfo.commandPool = CommandPool;
	CmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CmdBufferInfo.commandBufferCount = 1;

	auto error = vkAllocateCommandBuffers(Device, &CmdBufferInfo, &CommandBuffer);
	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}
}

void Renderer::DeleteCommandBuffer()
{
	// Did it this way because the samples from Lunarg are like this.
	VkCommandBuffer CmdBuf[1] = { CommandBuffer };
	vkFreeCommandBuffers(Device, CommandPool, 1, CmdBuf);
}

void Renderer::BeginCommandBuffer()
{
	VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
	CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufferBeginInfo.pNext = NULL;
	CmdBufferBeginInfo.flags = 0;
	CmdBufferBeginInfo.pInheritanceInfo = NULL;

	auto error = vkBeginCommandBuffer(CommandBuffer, &CmdBufferBeginInfo);

	if (error != VK_SUCCESS)
	{
		std::exit(-1);
	}
}

void Renderer::EndCommandBuffer()
{
	auto error = vkEndCommandBuffer(CommandBuffer);
	if (error != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::CreateDepthBuffer()
{
	VkImageCreateInfo ImageInfo = {};
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;
	VkFormatProperties FormatProperties;
	vkGetPhysicalDeviceFormatProperties(PhysicalDevice, depth_format, &FormatProperties);

	if (FormatProperties.linearTilingFeatures &VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		ImageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else {
		/* Try other depth formats? */
		std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
		exit(-1);
	}

	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageInfo.pNext = NULL;
	ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageInfo.format = depth_format;
	ImageInfo.extent.width = SurfaceSizeX;
	ImageInfo.extent.height = SurfaceSizeY;
	ImageInfo.extent.depth = 1;
	ImageInfo.mipLevels = 1;
	ImageInfo.arrayLayers = 1;
	ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	ImageInfo.queueFamilyIndexCount = 0;
	ImageInfo.pQueueFamilyIndices = NULL;
	ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageInfo.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = NULL;
	view_info.image = VK_NULL_HANDLE;
	view_info.format = depth_format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags = 0;
	//TODO Finish DepthBuffer

}


