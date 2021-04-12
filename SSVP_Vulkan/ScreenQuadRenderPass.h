#pragma once

#include "RenderPass.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include "Primitives.h"
#include <chrono>

extern const int MAX_FRAMES_IN_FLIGHT;

class Vulkan_Renderer;

class ScreenQuadRenderPass : public RenderPass {
public:
	ScreenQuadRenderPass(Vulkan_Renderer& renderer);
	~ScreenQuadRenderPass();

	virtual void RenderFrame() override;

	//some important parameter changed
	virtual void freeResources() override;
	virtual void recreateResources() override;

	void loadAssets();
	void createRenderPass();
	void createPipeline();
	void createFramebuffers();
	void createCommandBuffers();
	void createSemaphores();	
	

	void createImageDescriptorLayout();
	void createDescriptorLayout();

	void createUniformBuffers();
	void updateUniformBuffer(uint32_t index);
	void createDescriptorPool();
	
	void createDescriptorSets();	
	void createImageDescriptorSets();

    //https://vulkan-tutorial.com/Texture_mapping/Images
	//layout transitions

public:
	Vulkan_Renderer& m_renderer;
		
	std::vector<VkSemaphore> waitImageAvailable;
	std::vector<VkSemaphore> waitRenderFinished;

	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	VkPipeline m_ScreenQuadPipeline;
	std::vector<VkFramebuffer> m_swapChainFramebuffers;	
	std::vector<VkCommandBuffer> m_commandBuffers;	
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;

	VkDescriptorSetLayout m_descriptorSetLayout;	
	VkDescriptorSetLayout m_imageDescriptorSetLayout;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;

	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkDescriptorSet> m_ImageDescriptorSets;

	std::vector<Mesh> m_meshList;

	std::string model_path;
		
};
