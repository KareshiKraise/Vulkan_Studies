#pragma once

#include "RenderPass.h"

class Vulkan_Renderer;

class ClearScreenPass :public RenderPass
{
public:
	ClearScreenPass(Vulkan_Renderer& renderer);
	~ClearScreenPass();
	Vulkan_Renderer& m_renderer;
	virtual void RenderFrame() override;

	void createCommandBuffer();
	void recordCommandBuffers();
public:
	VkClearColorValue m_clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
	VkClearValue m_clearValue;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_cmdBufs;
	VkSemaphore m_renderWaitSemaphore;
	VkSemaphore m_imageAvailSemaphore;
};
