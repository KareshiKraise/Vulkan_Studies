#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include <chrono>

class Vulkan_Backend;
const int MAX_FRAMES_IN_FLIGHT = 2;

//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views


VkCommandBuffer beginSingleTimeCommands(Vulkan_Backend& backend);
void endSingleTimeCommands(Vulkan_Backend& backend, VkCommandBuffer cmdBuffer);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, Vulkan_Backend& backend);
void createBuffer(Vulkan_Backend& backend, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void copyBuffer(Vulkan_Backend& backend, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool);
void createImage(Vulkan_Backend& backend, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
void copyBufferToImage(Vulkan_Backend& backend,
	VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
VkImageView createImageView(Vulkan_Backend& backend, VkImage image, VkFormat format);


void transitionImageLayout(Vulkan_Backend& backend,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout); 

class RenderPass;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return (graphicsFamily.has_value() && presentFamily.has_value());
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SwapChain_ParamsAndData {
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
};

struct SurfaceParams {
	bool resized;
};

extern SurfaceParams surfParams;

class Vulkan_Backend
{
public:
	Vulkan_Backend(const Vulkan_Backend&) = delete;
	Vulkan_Backend();
	~Vulkan_Backend();

	void initWindow();
	void initVulkan();
	void cleanUp();
	
	void createInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSurface();
	void createCommandPool();
	
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapChain();
	void createImageViews();	
	void recreateSwapChain();
	void cleanupSwapChain();

	GLFWwindow* m_window;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapChain;
	SwapChain_ParamsAndData m_swapChainParams;
	QueueFamilyIndices m_queueFamily;
	SurfaceParams m_surfParams{false};
	VkCommandPool m_commandPool;
	VkDescriptorPool m_descriptorPool;
		
	int m_width;
	int m_height;
};

struct defaultPassSemaphores {
	VkSemaphore waitImageAvailable;
	VkSemaphore waitRenderFinished;	
};

class Vulkan_Renderer
{
public:
	Vulkan_Renderer();
	~Vulkan_Renderer();

	Vulkan_Backend m_backend;
	VkViewport m_viewport;
	size_t currentFrame = 0;
	std::chrono::steady_clock::time_point startTime;
	double elapsedTime;
	double frameTime;
	

	void mainLoop(RenderPass* pass);
};