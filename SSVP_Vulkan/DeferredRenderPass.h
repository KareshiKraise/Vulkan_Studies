#pragma once

#include "RenderPass.h"
#include "Renderer.h"
#include <glm/glm.hpp>

#include "VertexBuffer.h"

class Vulkan_Renderer;

class DeferredRenderPass : public RenderPass
{
public:
	DeferredRenderPass(Vulkan_Renderer& renderer);
	~DeferredRenderPass();

	virtual void RenderFrame() override;
	virtual void freeResources() override;
	virtual void recreateResources() override;
	
public:
	Vulkan_Renderer& m_renderer;
		

	struct framebufferAttachment{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
	};

	struct framebuffer {
		int32_t width, height;
		VkFramebuffer framebuffer;
		framebufferAttachment pos, norm, col;
		framebufferAttachment depth;
		VkRenderPass renderPass;
	} m_offScreenFramebuffer;

	VkSampler m_texSampler;

	VkRenderPass m_renderPass;

	VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
	VkSemaphore m_deferredSemaphore = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_deferredPipeline;
	VkPipeline m_presentPipeline;

public: 
	void createAttachment(VkFormat format, VkImageUsageFlagBits usage, framebufferAttachment* attachment);
	void createPipeline();
	void createFramebuffer();
};