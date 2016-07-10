/*
* Most of this code is based of the LunarG samples.
*
*/

#include "Renderer.h"
#include <iostream>
#include <sstream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#ifdef _WIN32
#include <Windows.h>
#endif


static const char *vertShaderText =
"#version 400\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
"layout (std140, binding = 0) uniform bufferVals {\n"
"    mat4 mvp;\n"
"} myBufferVals;\n"
"layout (location = 0) in vec4 pos;\n"
"layout (location = 1) in vec4 inColor;\n"
"layout (location = 0) out vec4 outColor;\n"
"out gl_PerVertex { \n"
"    vec4 gl_Position;\n"
"};\n"
"void main() {\n"
"   outColor = inColor;\n"
"   gl_Position = myBufferVals.mvp * pos;\n"
"}\n";

static const char *fragShaderText =
"#version 400\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
"layout (location = 0) in vec4 color;\n"
"layout (location = 0) out vec4 outColor;\n"
"void main() {\n"
"   outColor = color;\n"
"}\n";

Renderer::Renderer()
{

	SurfaceSizeX = 1920;
	SurfaceSizeY = 1080;
	// Set up debug layers.
	//SetupDebug();
	// Init GLFW for WSI help.
	InitGLFW();
	// Get Vulkan Instance
	InitInstance();
	// Init Lunarg debug layers.
	//InitDebug();
	// Init and grab device and physical device.
	InitDevice();
	// Create surface.
	GLFWCreateSurface();
	// Create commandpool.
	InitCommandPool();
	// Create command buffer.
	InitCommandBuffer();
	// Create swapchain for swapimages.
	InitSwapchain();
	// Create Images to swap.
	InitSwapImages();
	// Begin accepting commands to the buffer.
	BeginCommandBuffer();
	// Create depth buffer.
	CreateDepthBuffer();

	InitUniformBuffer();

	InitDescriptorPipelineLayout(false);

	InitRenderpass(true, true);
	InitShaders(vertShaderText, fragShaderText);
	InitFramebuffer(true);
	InitVertexBuffer(g_vb_solid_face_colors_Data,
		sizeof(g_vb_solid_face_colors_Data),
		sizeof(g_vb_solid_face_colors_Data[0]), false);
	uint32_t size = sizeof(Vertex);
	InitDescriptorPool(false);
	InitDescriptorSet(false);
	InitPipelineCache();
	InitGraphicsPipeline(true, true);
	//ExecuteQueueCommandBuffer();
	//InitSemaphore();

	FlushCommandBuffer();

	CreateFence();
	NBFrames = 0;
	LastTime = glfwGetTime();
	while (true)
	{
		DrawCube();
		CalcMS();
		
	}
}


Renderer::~Renderer()
{
	DeleteGraphcisPipeline();
	DeletePipelineCache();
	DeleteDescriptorPool();
	DeleteVertexBuffer();
	DeleteFramebuffer();
	DeleteShaders();
	DeleteRenderpass();
	DeleteDescriptorPipelineLayout();
	DeleteUniformBuffer();
	DeleteDepthBuffer();
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

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
	/*
	VkPhysicalDeviceFeatures Features;
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);

	VkPhysicalDeviceProperties DeviceProps;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProps);
	*/

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

/*
* This are pointers to extension functions.
*/
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

	//GLFW adds this extension.
	//InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

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
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// This tells GLFW to not create an OpenGL context with the window
	window = glfwCreateWindow(SurfaceSizeX, SurfaceSizeY, "Learn Vulkan", nullptr, nullptr);

	// make sure we indeed get the surface size we want.
	glfwGetFramebufferSize(window, &SurfaceSizeX, &SurfaceSizeY);


	VkResult ret = glfwCreateWindowSurface(Instance, window, nullptr, &Surface);
	if (VK_SUCCESS != ret) {
		// couldn't create surface, exit
		glfwTerminate();
		std::exit(-1);
	}

	VkBool32 WSI_supported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GraphicsFamilyIndex, Surface, &WSI_supported);
	if (!WSI_supported)
	{ // Window System Integration not supported WHAT?!?! 
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
	CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

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


	VkMemoryRequirements mem_reqs;

	DepthFormat = depth_format;

	auto error = vkCreateImage(Device, &ImageInfo, NULL, &DepthImage);
	if (error != VK_SUCCESS)
		std::exit(-1);

	vkGetImageMemoryRequirements(Device, DepthImage, &mem_reqs);

	mem_alloc.allocationSize = mem_reqs.size;

	// From Lunarg samples
	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		0, /* No Requirements */
		&mem_alloc.memoryTypeIndex);

	if (!pass)
		std::exit(-1);

	error = vkAllocateMemory(Device, &mem_alloc, nullptr, &DepthMemory);
	if (error != VK_SUCCESS)
		std::exit(-1);

	error = vkBindImageMemory(Device, DepthImage, DepthMemory, 0);
	if (error != VK_SUCCESS)
		std::exit(-1);

	set_image_layout(DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	view_info.image = DepthImage;
	error = vkCreateImageView(Device, &view_info, NULL, &DepthImageView);
	if (error != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::DeleteDepthBuffer()
{
	vkDestroyImageView(Device, DepthImageView, NULL);
	vkDestroyImage(Device, DepthImage, NULL);
	vkFreeMemory(Device, DepthMemory, NULL);
}

// From Lunarg samples.
bool Renderer::memory_type_from_properties(uint32_t typeBits,
	VkFlags requirements_mask,
	uint32_t *typeIndex)
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((MemoryProperties.memoryTypes[i].propertyFlags &
				requirements_mask) == requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

// Thank you lunarg.
void Renderer::set_image_layout(VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout old_image_layout,
	VkImageLayout new_image_layout) {
	/* DEPENDS on info.cmd and info.queue initialized */

	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = old_image_layout;
	image_memory_barrier.newLayout = new_image_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = aspectMask;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.srcAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		image_memory_barrier.srcAccessMask =
			VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;


	vkCmdPipelineBarrier(CommandBuffer, src_stages, dest_stages, 0, 0, NULL, 0, NULL,
		1, &image_memory_barrier);
}

void Renderer::EndCommandBuffer()
{
	auto res = vkEndCommandBuffer(CommandBuffer);
	if (res != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::ResetCommandBuffer()
{
	vkResetCommandBuffer(CommandBuffer, 0);
}

void Renderer::FlushCommandBuffer()
{
	EndCommandBuffer();
	ExecuteQueueCommandBuffer();
}

/*
* TODO Split into different functions.
*/
void Renderer::ExecuteQueueCommandBuffer()
{

	/* Queue the command buffer for execution */
	const VkCommandBuffer cmd_bufs[] = { CommandBuffer };
	VkFenceCreateInfo fenceInfo;
	VkFence drawFence;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = NULL;
	fenceInfo.flags = 0;
	vkCreateFence(Device, &fenceInfo, NULL, &drawFence);

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submit_info[1] = {};
	submit_info[0].pNext = NULL;
	submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info[0].waitSemaphoreCount = 0;
	submit_info[0].pWaitSemaphores = NULL;
	submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
	submit_info[0].commandBufferCount = 1;
	submit_info[0].pCommandBuffers = cmd_bufs;
	submit_info[0].signalSemaphoreCount = 0;
	submit_info[0].pSignalSemaphores = NULL;

	auto error = vkQueueSubmit(Queue, 1, submit_info, drawFence);
	if (error != VK_SUCCESS)
		std::exit(-1);

	VkResult result;
	do
	{
		result = vkWaitForFences(Device, 1, &drawFence, VK_TRUE, UINT64_MAX);
	} while (result == VK_TIMEOUT);
	
	
	//vkDestroyFence(Device, drawFence, NULL);
}

void Renderer::InitUniformBuffer()
{

	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)SurfaceSizeY / (float)SurfaceSizeX, 0.1f, 100.0f);
	glm::mat4 View = glm::lookAt(
		glm::vec3(0, 20, 4), // Camera is at (0,3,10), in World Space
		glm::vec3(0, 0, 0),  // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	glm::mat4 Model = glm::mat4
	(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	Model = glm::translate(Model, glm::vec3(0, 5, 0));

	glm::mat4 MVP = Projection * View * Model;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(MVP);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	auto res = vkCreateBuffer(Device, &buf_info, NULL, &UniformBuffer);
	if (res != VK_SUCCESS)
		std::exit(-1);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(Device, UniformBuffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;
	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);

	if (!pass)
		std::exit(-1);

	res = vkAllocateMemory(Device, &alloc_info, NULL, &UniformMemory);
	if (res != VK_SUCCESS)
		std::exit(-1);

	uint8_t *pData;
	res = vkMapMemory(Device, UniformMemory, 0, mem_reqs.size, 0, (void **)&pData);

	memcpy(pData, &MVP, sizeof(MVP));

	vkUnmapMemory(Device, UniformMemory);

	res = vkBindBufferMemory(Device, UniformBuffer, UniformMemory, 0);
	if (res != VK_SUCCESS)
		std::exit(-1);

	UniformDescriptor.buffer = UniformBuffer;
	UniformDescriptor.offset = 0;
	UniformDescriptor.range = sizeof(MVP);
}

void Renderer::DeleteUniformBuffer()
{
	vkDestroyBuffer(Device, UniformBuffer, NULL);
	vkFreeMemory(Device, UniformMemory, NULL);
}

void Renderer::InitDescriptorPipelineLayout(bool UseTexture)
{
	VkDescriptorSetLayoutBinding LayoutBindings[2];
	// Tell the pipeline to link uniform buffer and vertex shader. 
	LayoutBindings[0].binding = 0;
	LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LayoutBindings[0].descriptorCount = 1;
	LayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	LayoutBindings[0].pImmutableSamplers = NULL;

	if (UseTexture)
	{
		// Tell the pipeline we have a texture for our fragment shader.
		LayoutBindings[1].binding = 1;
		LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		LayoutBindings[1].descriptorCount = 1;
		LayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		LayoutBindings[1].pImmutableSamplers = NULL;
	}

	VkDescriptorSetLayoutCreateInfo DescriptorLayout = {};
	DescriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	DescriptorLayout.pNext = NULL;
	DescriptorLayout.bindingCount = UseTexture ? 2 : 1; // Change binding count if we use texture or not.
	DescriptorLayout.pBindings = LayoutBindings;

	DescriptorSetLayouts.resize(1);
	auto res = vkCreateDescriptorSetLayout(Device, &DescriptorLayout, NULL, DescriptorSetLayouts.data());
	if (res != VK_SUCCESS)
		std::exit(-1);

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.pNext = NULL;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	PipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	PipelineLayoutCreateInfo.setLayoutCount = 1;
	PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts.data();

	res = vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, NULL, &PipelineLayout);
	if (res != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::DeleteDescriptorPipelineLayout()
{
	// We have only one DescriptorSetLayout, but may have more in the future.
	for (int i = 0; i < 1; i++)
	{
		vkDestroyDescriptorSetLayout(Device, DescriptorSetLayouts[i], NULL);
		vkDestroyPipelineLayout(Device, PipelineLayout, NULL);
	}
}

void Renderer::InitRenderpass(bool clear, bool UseDepth)
{
	// We have two attachments one color and the depthstencil.
	VkAttachmentDescription Attachments[2];
	Attachments[0].format = SurfaceFormat.format;
	Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[0].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].flags = 0;

	if (UseDepth) {
		Attachments[1].format = DepthFormat;
		Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		Attachments[1].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		Attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		Attachments[1].flags = 0;
	}

	VkAttachmentReference color_reference = {};
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = UseDepth ? &depth_reference : NULL;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.pNext = NULL;
	rp_info.attachmentCount = UseDepth ? 2 : 1;
	rp_info.pAttachments = Attachments;
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = NULL;

	auto res = vkCreateRenderPass(Device, &rp_info, NULL, &RenderPass);

	if (res != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::DeleteRenderpass()
{
	vkDestroyRenderPass(Device, RenderPass, NULL);
}

void Renderer::InitShaders(const char * VertShader, const char * FragShader)
{

	if (!(VertShader || FragShader))
		return;

	glslang::InitializeProcess();
	VkShaderModuleCreateInfo moduleCreateInfo;

	if (VertShader) {
		std::vector<unsigned int> vtx_spv;
		ShaderStages[0].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ShaderStages[0].pNext = NULL;
		ShaderStages[0].pSpecializationInfo = NULL;
		ShaderStages[0].flags = 0;
		ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		ShaderStages[0].pName = "main";

		auto retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, VertShader, vtx_spv);
		assert(retVal);

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
		moduleCreateInfo.pCode = vtx_spv.data();
		auto res = vkCreateShaderModule(Device, &moduleCreateInfo, NULL,
			&ShaderStages[0].module);
		assert(res == VK_SUCCESS);
	}

	if (FragShader) {
		std::vector<unsigned int> frag_spv;
		ShaderStages[1].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ShaderStages[1].pNext = NULL;
		ShaderStages[1].pSpecializationInfo = NULL;
		ShaderStages[1].flags = 0;
		ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		ShaderStages[1].pName = "main";

		auto retVal =
			GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader, frag_spv);
		assert(retVal);

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
		moduleCreateInfo.pCode = frag_spv.data();
		auto res = vkCreateShaderModule(Device, &moduleCreateInfo, NULL,
			&ShaderStages[1].module);
		assert(res == VK_SUCCESS);
	}

	glslang::FinalizeProcess();
}

void Renderer::DeleteShaders()
{
	vkDestroyShaderModule(Device, ShaderStages[0].module, NULL);
	vkDestroyShaderModule(Device, ShaderStages[1].module, NULL);
}

void Renderer::InitFramebuffer(bool UseDepth)
{
	// Finaly we can use our depthImageView
	// Our two attachments are the 1 color and depth stencil.
	VkImageView attachments[2];
	attachments[1] = DepthImageView;

	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = RenderPass;
	fb_info.attachmentCount = UseDepth ? 2 : 1;
	fb_info.pAttachments = attachments;
	fb_info.width = SurfaceSizeX;
	fb_info.height = SurfaceSizeY;
	fb_info.layers = 1;

	// This is just C malloc. Maybe change to new.
	framebuffers = (VkFramebuffer *)malloc(SwapchainImageCount * sizeof(VkFramebuffer));

	VkResult res;
	uint32_t i;
	for (i = 0; i < SwapchainImageCount; i++)
	{
		attachments[0] = SwapchainImageViews[i];
		res = vkCreateFramebuffer(Device, &fb_info, NULL, &framebuffers[i]);
		if (res != VK_SUCCESS)
			std::exit(-1);
	}
}

void Renderer::DeleteFramebuffer()
{
	for (uint32_t i = 0; i < SwapchainImageCount; i++) {
		vkDestroyFramebuffer(Device, framebuffers[i], NULL);
	}
	free(framebuffers);
}

void Renderer::InitVertexBuffer(const void * vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture)
{
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buf_info.size = dataSize;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	auto res = vkCreateBuffer(Device, &buf_info, NULL, &VertexBuffer);
	if (res != VK_SUCCESS)
		std::exit(-1);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(Device, VertexBuffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;
	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);

	if (!pass)
		std::exit(-1);

	res = vkAllocateMemory(Device, &alloc_info, NULL, &VertexBufferMemory);
	if (res != VK_SUCCESS)
		std::exit(-1);

	VertexBufferInfo.range = mem_reqs.size;
	VertexBufferInfo.offset = 0;

	uint8_t *pData;
	res = vkMapMemory(Device, VertexBufferMemory, 0, mem_reqs.size, 0, (void **)&pData);
	if (res != VK_SUCCESS)
		std::exit(-1);
	
	memcpy(pData, vertexData, dataSize);

	vkUnmapMemory(Device, VertexBufferMemory);

	res = vkBindBufferMemory(Device, VertexBuffer, VertexBufferMemory, 0);
	if (res != VK_SUCCESS)
		std::exit(-1);

	VertexInputBindingDesc.binding = 0;
	VertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDesc.stride = dataStride;

	VertexInputAttributeDesc[0].binding = 0;
	VertexInputAttributeDesc[0].location = 0;
	VertexInputAttributeDesc[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	VertexInputAttributeDesc[0].offset = 0;
	VertexInputAttributeDesc[1].binding = 0;
	VertexInputAttributeDesc[1].location = 1;
	VertexInputAttributeDesc[1].format = use_texture ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
	VertexInputAttributeDesc[1].offset = 16;
}

void Renderer::DeleteVertexBuffer()
{
	vkDestroyBuffer(Device, VertexBuffer, NULL);
	vkFreeMemory(Device, VertexBufferMemory, NULL);
}

void Renderer::InitDescriptorPool(bool UseTexture)
{
	VkDescriptorPoolSize type_count[2];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;
	if (UseTexture) {
		type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		type_count[1].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo descriptor_pool = {};
	descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool.pNext = NULL;
	descriptor_pool.maxSets = 1;
	descriptor_pool.poolSizeCount = UseTexture ? 2 : 1;
	descriptor_pool.pPoolSizes = type_count;

	auto res = vkCreateDescriptorPool(Device, &descriptor_pool, NULL, &DescriptorPool);
	if (res != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::DeleteDescriptorPool()
{
	vkDestroyDescriptorPool(Device, DescriptorPool, NULL);
}

void Renderer::InitDescriptorSet(bool UseTexture)
{
	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = NULL;
	alloc_info[0].descriptorPool = DescriptorPool;
	alloc_info[0].descriptorSetCount = 1;
	alloc_info[0].pSetLayouts = DescriptorSetLayouts.data();

	DescriptorSet.resize(1);
	auto res = vkAllocateDescriptorSets(Device, alloc_info, DescriptorSet.data());
	if (res != VK_SUCCESS)
		std::exit(-1);

	VkWriteDescriptorSet writes[2];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = DescriptorSet[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &UniformDescriptor;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

	if (UseTexture) {
		writes[1] = {};
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = DescriptorSet[0];
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		//writes[1].pImageInfo = &;
		writes[1].dstArrayElement = 0;
	}

	vkUpdateDescriptorSets(Device, UseTexture ? 2 : 1, writes, 0, NULL);
}

void Renderer::InitPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCache;
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCache.pNext = NULL;
	pipelineCache.initialDataSize = 0;
	pipelineCache.pInitialData = NULL;
	pipelineCache.flags = 0;
	auto res = vkCreatePipelineCache(Device, &pipelineCache, NULL, &PipelineCache);
}

void Renderer::DeletePipelineCache()
{
	vkDestroyPipelineCache(Device, PipelineCache, NULL);
}

void Renderer::InitGraphicsPipeline(VkBool32 include_depth, VkBool32 include_vi)
{
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = NULL;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = 0;

	VkPipelineVertexInputStateCreateInfo vi;
	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (include_vi) {
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &VertexInputBindingDesc;
		vi.vertexAttributeDescriptionCount = 2;
		vi.pVertexAttributeDescriptions = VertexInputAttributeDesc;
	}

	VkPipelineInputAssemblyStateCreateInfo ia;
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.pNext = NULL;
	ia.flags = 0;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rs;
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.pNext = NULL;
	rs.flags = 0;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rs.depthClampEnable = include_depth;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 1.0f;

	VkPipelineColorBlendStateCreateInfo cb;
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.flags = 0;
	cb.pNext = NULL;

	VkPipelineColorBlendAttachmentState att_state[1];
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;
	cb.logicOpEnable = VK_FALSE;
	cb.logicOp = VK_LOGIC_OP_NO_OP;
	cb.blendConstants[0] = 1.0f;
	cb.blendConstants[1] = 1.0f;
	cb.blendConstants[2] = 1.0f;
	cb.blendConstants[3] = 1.0f;

	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.pNext = NULL;
	vp.flags = 0;

	vp.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_SCISSOR;
	vp.pScissors = NULL;
	vp.pViewports = NULL;

	VkPipelineDepthStencilStateCreateInfo ds;
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.pNext = NULL;
	ds.flags = 0;
	ds.depthTestEnable = include_depth;
	ds.depthWriteEnable = include_depth;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.stencilTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.back.compareMask = 0;
	ds.back.reference = 0;
	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
	ds.back.writeMask = 0;
	ds.minDepthBounds = 0;
	ds.maxDepthBounds = 0;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	VkPipelineMultisampleStateCreateInfo ms;
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pNext = NULL;
	ms.flags = 0;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE;
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0;

	VkGraphicsPipelineCreateInfo pipeline;
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.pNext = NULL;
	pipeline.layout = PipelineLayout;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;
	pipeline.flags = 0;
	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pTessellationState = NULL;
	pipeline.pMultisampleState = &ms;
	pipeline.pDynamicState = &dynamicState;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.pStages = ShaderStages;
	pipeline.stageCount = 2;
	pipeline.renderPass = RenderPass;
	pipeline.subpass = 0;

	auto res = vkCreateGraphicsPipelines(Device, PipelineCache, 1, &pipeline, NULL, &GraphicsPipeline);

	if (res != VK_SUCCESS)
		std::exit(-1);
}

void Renderer::DeleteGraphcisPipeline()
{
	vkDestroyPipeline(Device, GraphicsPipeline, NULL);
}

bool Renderer::GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader,
	std::vector<unsigned int> &spirv) {

	EShLanguage stage = FindLanguage(shader_type);
	glslang::TShader shader(stage);
	glslang::TProgram program;
	const char *shaderStrings[1];
	TBuiltInResource Resources;
	init_resources(Resources);

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	shaderStrings[0] = pshader;
	shader.setStrings(shaderStrings, 1);

	if (!shader.parse(&Resources, 100, false, messages)) {
		puts(shader.getInfoLog());
		puts(shader.getInfoDebugLog());
		return false; // something didn't work
	}

	program.addShader(&shader);

	//
	// Program-level processing...
	//

	if (!program.link(messages)) {
		puts(shader.getInfoLog());
		puts(shader.getInfoDebugLog());
		fflush(stdout);
		return false;
	}

	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

	return true;
}

EShLanguage Renderer::FindLanguage(const VkShaderStageFlagBits shader_type) {
	switch (shader_type) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return EShLangVertex;

	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return EShLangTessControl;

	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return EShLangTessEvaluation;

	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return EShLangGeometry;

	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return EShLangFragment;

	case VK_SHADER_STAGE_COMPUTE_BIT:
		return EShLangCompute;

	default:
		return EShLangVertex;
	}
}

void Renderer::init_resources(TBuiltInResource &Resources)
{
	Resources.maxLights = 32;
	Resources.maxClipPlanes = 6;
	Resources.maxTextureUnits = 32;
	Resources.maxTextureCoords = 32;
	Resources.maxVertexAttribs = 64;
	Resources.maxVertexUniformComponents = 4096;
	Resources.maxVaryingFloats = 64;
	Resources.maxVertexTextureImageUnits = 32;
	Resources.maxCombinedTextureImageUnits = 80;
	Resources.maxTextureImageUnits = 32;
	Resources.maxFragmentUniformComponents = 4096;
	Resources.maxDrawBuffers = 32;
	Resources.maxVertexUniformVectors = 128;
	Resources.maxVaryingVectors = 8;
	Resources.maxFragmentUniformVectors = 16;
	Resources.maxVertexOutputVectors = 16;
	Resources.maxFragmentInputVectors = 15;
	Resources.minProgramTexelOffset = -8;
	Resources.maxProgramTexelOffset = 7;
	Resources.maxClipDistances = 8;
	Resources.maxComputeWorkGroupCountX = 65535;
	Resources.maxComputeWorkGroupCountY = 65535;
	Resources.maxComputeWorkGroupCountZ = 65535;
	Resources.maxComputeWorkGroupSizeX = 1024;
	Resources.maxComputeWorkGroupSizeY = 1024;
	Resources.maxComputeWorkGroupSizeZ = 64;
	Resources.maxComputeUniformComponents = 1024;
	Resources.maxComputeTextureImageUnits = 16;
	Resources.maxComputeImageUniforms = 8;
	Resources.maxComputeAtomicCounters = 8;
	Resources.maxComputeAtomicCounterBuffers = 1;
	Resources.maxVaryingComponents = 60;
	Resources.maxVertexOutputComponents = 64;
	Resources.maxGeometryInputComponents = 64;
	Resources.maxGeometryOutputComponents = 128;
	Resources.maxFragmentInputComponents = 128;
	Resources.maxImageUnits = 8;
	Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	Resources.maxCombinedShaderOutputResources = 8;
	Resources.maxImageSamples = 0;
	Resources.maxVertexImageUniforms = 0;
	Resources.maxTessControlImageUniforms = 0;
	Resources.maxTessEvaluationImageUniforms = 0;
	Resources.maxGeometryImageUniforms = 0;
	Resources.maxFragmentImageUniforms = 8;
	Resources.maxCombinedImageUniforms = 8;
	Resources.maxGeometryTextureImageUnits = 16;
	Resources.maxGeometryOutputVertices = 256;
	Resources.maxGeometryTotalOutputComponents = 1024;
	Resources.maxGeometryUniformComponents = 1024;
	Resources.maxGeometryVaryingComponents = 64;
	Resources.maxTessControlInputComponents = 128;
	Resources.maxTessControlOutputComponents = 128;
	Resources.maxTessControlTextureImageUnits = 16;
	Resources.maxTessControlUniformComponents = 1024;
	Resources.maxTessControlTotalOutputComponents = 4096;
	Resources.maxTessEvaluationInputComponents = 128;
	Resources.maxTessEvaluationOutputComponents = 128;
	Resources.maxTessEvaluationTextureImageUnits = 16;
	Resources.maxTessEvaluationUniformComponents = 1024;
	Resources.maxTessPatchComponents = 120;
	Resources.maxPatchVertices = 32;
	Resources.maxTessGenLevel = 64;
	Resources.maxViewports = 16;
	Resources.maxVertexAtomicCounters = 0;
	Resources.maxTessControlAtomicCounters = 0;
	Resources.maxTessEvaluationAtomicCounters = 0;
	Resources.maxGeometryAtomicCounters = 0;
	Resources.maxFragmentAtomicCounters = 8;
	Resources.maxCombinedAtomicCounters = 8;
	Resources.maxAtomicCounterBindings = 1;
	Resources.maxVertexAtomicCounterBuffers = 0;
	Resources.maxTessControlAtomicCounterBuffers = 0;
	Resources.maxTessEvaluationAtomicCounterBuffers = 0;
	Resources.maxGeometryAtomicCounterBuffers = 0;
	Resources.maxFragmentAtomicCounterBuffers = 1;
	Resources.maxCombinedAtomicCounterBuffers = 1;
	Resources.maxAtomicCounterBufferSize = 16384;
	Resources.maxTransformFeedbackBuffers = 4;
	Resources.maxTransformFeedbackInterleavedComponents = 64;
	Resources.maxCullDistances = 8;
	Resources.maxCombinedClipAndCullDistances = 8;
	Resources.maxSamples = 4;
	Resources.limits.nonInductiveForLoops = 1;
	Resources.limits.whileLoops = 1;
	Resources.limits.doWhileLoops = 1;
	Resources.limits.generalUniformIndexing = 1;
	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
	Resources.limits.generalVaryingIndexing = 1;
	Resources.limits.generalSamplerIndexing = 1;
	Resources.limits.generalVariableIndexing = 1;
	Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

void Renderer::DrawCube()
{
	vkDeviceWaitIdle(Device);

	InitSemaphore();


	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 0.2f;
	clear_values[0].color.float32[1] = 0.2f;
	clear_values[0].color.float32[2] = 0.2f;
	clear_values[0].color.float32[3] = 0.2f;
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	
	// Get the index of the next available swapchain image:
	auto res = vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX,
		presentCompleteSemaphore, VK_NULL_HANDLE,
		&CurrentBuffer);

	BeginCommandBuffer();

	set_image_layout(SwapchainImages[CurrentBuffer],
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderPassBeginInfo rp_begin;
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.pNext = NULL;
	rp_begin.renderPass = RenderPass;
	rp_begin.framebuffer = framebuffers[CurrentBuffer];
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = SurfaceSizeX;
	rp_begin.renderArea.extent.height = SurfaceSizeY;
	rp_begin.clearValueCount = 2;
	rp_begin.pClearValues = clear_values;


	vkCmdBeginRenderPass(CommandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		PipelineLayout, 0, 1,
		DescriptorSet.data(), 0, NULL);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBuffer, offsets);

	VkViewport Viewport;
	Viewport.height = (float)SurfaceSizeX;
	Viewport.width = (float)SurfaceSizeY;
	Viewport.minDepth = (float)0.0f;
	Viewport.maxDepth = (float)1.0f;
	Viewport.x = 0;
	Viewport.y = 0;
	vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

	VkRect2D Scissor;
	Scissor.extent.width = SurfaceSizeX;
	Scissor.extent.height = SurfaceSizeY;
	Scissor.offset.x = 0;
	Scissor.offset.y = 0;
	vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

	vkCmdDraw(CommandBuffer, 12 * 3, 1, 0, 0);

	vkCmdEndRenderPass(CommandBuffer);

	VkImageMemoryBarrier prePresentBarrier = {};
	prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	prePresentBarrier.pNext = NULL;
	prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	prePresentBarrier.subresourceRange.baseMipLevel = 0;
	prePresentBarrier.subresourceRange.levelCount = 1;
	prePresentBarrier.subresourceRange.baseArrayLayer = 0;
	prePresentBarrier.subresourceRange.layerCount = 1;
	prePresentBarrier.image = SwapchainImages[CurrentBuffer];

	vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
		NULL, 1, &prePresentBarrier);

	res = vkEndCommandBuffer(CommandBuffer);

	if (res != VK_SUCCESS)
		std::exit(-1);

	const VkCommandBuffer cmd_bufs[] = { CommandBuffer };

	VkPipelineStageFlags pipe_stage_flags =
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submit_info[1] = {};
	submit_info[0].pNext = NULL;
	submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info[0].waitSemaphoreCount = 1;
	submit_info[0].pWaitSemaphores = &presentCompleteSemaphore;
	submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
	submit_info[0].commandBufferCount = 1;
	submit_info[0].pCommandBuffers = cmd_bufs;
	submit_info[0].signalSemaphoreCount = 0;
	submit_info[0].pSignalSemaphores = NULL;

	res = vkQueueSubmit(Queue, 1, submit_info, drawFence);
	if (res != VK_SUCCESS)
		std::exit(-1);

	//vkQueueWaitIdle(Queue);
	VkResult result;
	do
	{
		result = vkWaitForFences(Device, 1, &drawFence, VK_TRUE, UINT64_MAX);
	} while (result == VK_TIMEOUT);


	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &Swapchain;
	present.pImageIndices = &CurrentBuffer;
	present.pWaitSemaphores = NULL;
	present.waitSemaphoreCount = 0;
	present.pResults = NULL;


	res = vkQueuePresentKHR(Queue, &present);

	vkResetFences(Device, 1, &drawFence);
	DeleteSemaphore();
	
	glfwPollEvents();
}

void Renderer::CreateFence()
{
	VkFenceCreateInfo fenceInfo;

	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = NULL;
	fenceInfo.flags = 0;
	vkCreateFence(Device, &fenceInfo, NULL, &drawFence);
}

void Renderer::InitSemaphore()
{
	VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
	presentCompleteSemaphoreCreateInfo.sType =
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	presentCompleteSemaphoreCreateInfo.pNext = NULL;
	presentCompleteSemaphoreCreateInfo.flags = 0;

	auto res = vkCreateSemaphore(Device, &presentCompleteSemaphoreCreateInfo,
		NULL, &presentCompleteSemaphore);
	assert(res == VK_SUCCESS);
}

void Renderer::DeleteSemaphore()
{
	vkDestroySemaphore(Device, presentCompleteSemaphore, NULL);
}

void Renderer::CalcMS()
{
	CurrentTime = glfwGetTime();
	NBFrames++;
	if (CurrentTime - LastTime >= 1.0) {
		printf("%f ms/frame FPS: %d\n",1000/ double(NBFrames),NBFrames);

		NBFrames = 0;
		LastTime += 1.0;
	}
}
