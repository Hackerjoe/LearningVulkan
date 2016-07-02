#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <cstdlib>
#include <GLFW\glfw3.h>
#include "SPIRV\GlslangToSpv.h"
#include "Cube.h"
#ifndef NOMINMAX
#define NOMINMAX /* Don't let Windows define min() or max() */
#endif
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
	void ResetCommandBuffer();

	void CreateDepthBuffer();
	void DeleteDepthBuffer();
	void ExecuteQueueCommandBuffer();

	void InitUniformBuffer();
	void DeleteUniformBuffer();

	void InitDescriptorPipelineLayout(bool UseTexture);
	void DeleteDescriptorPipelineLayout();

	void InitRenderpass(bool clear, bool UseDepth);
	void DeleteRenderpass();

	void InitShaders(const char* VertShader, const char* FragShader);
	void DeleteShaders();

	void InitFramebuffer(bool UseDepth);
	void DeleteFramebuffer();

	void InitVertexBuffer(const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture);
	void DeleteVertexBuffer();

	void InitDescriptorPool(bool UseTexture);
	void DeleteDescriptorPool();

	void InitDescriptorSet(bool UseTexture);

	void InitPipelineCache();
	void DeletePipelineCache();

	void InitGraphicsPipeline(VkBool32 include_depth, VkBool32 include_vi);
	void DeleteGraphcisPipeline();

	void DrawCube();

	void CreateFence();
	void DeleteFence();

	void InitSemaphore();
	void DeleteSemaphore();
	/*
	Functions from lunarg samples.
	*/
	bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
	void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout);
	bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);
	EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
	void init_resources(TBuiltInResource &Resources);
	///////////////////
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	VkPhysicalDeviceProperties DeviceProperties;

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

	int SurfaceSizeX = 1920;
	int SurfaceSizeY = 1080;

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

	//Depth Buffer
	VkFormat DepthFormat;
	VkImage DepthImage;
	VkDeviceMemory DepthMemory;
	VkImageView DepthImageView;

	// Uniform Buffer
	VkBuffer UniformBuffer;
	VkDeviceMemory UniformMemory;
	VkDescriptorBufferInfo UniformDescriptor;

	//Pipeline Descriptor Layout
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	VkPipelineLayout PipelineLayout;

	//Renderpass
	VkRenderPass RenderPass;

	//Shader stuff
	VkPipelineShaderStageCreateInfo ShaderStages[2];

	//Framebuffer
	VkFramebuffer *framebuffers;

	//Vertex Data for Cube
	VkBuffer VertexBuffer;
	VkDeviceMemory VertexBufferMemory;
	VkDescriptorBufferInfo VertexBufferInfo;
	VkVertexInputBindingDescription VertexInputBindingDesc;
	VkVertexInputAttributeDescription VertexInputAttributeDesc[2];

	//
	VkDescriptorPool DescriptorPool = nullptr;

	//
	std::vector<VkDescriptorSet> DescriptorSet;

	//
	VkPipelineCache PipelineCache = nullptr;
	VkPipeline GraphicsPipeline = nullptr;

	uint32_t CurrentBuffer;

	//
	VkSemaphore presentCompleteSemaphore;
	//
	VkFence drawFence;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
};