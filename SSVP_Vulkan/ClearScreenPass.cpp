#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include "ClearScreenPass.h"
#include <stdexcept>
#include "Renderer.h"
#include <iostream>

//CTRL+M+O fold all

ClearScreenPass::ClearScreenPass(Vulkan_Renderer& renderer) :
	m_renderer{ renderer }, m_cmdBufs{ 0 }, m_commandPool{ 0 }
{
	m_clearValue.color = m_clearColor;

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkResult res1 = vkCreateSemaphore(m_renderer.m_backend.m_device, &semaphoreInfo, nullptr, &m_renderWaitSemaphore);
	VkResult res2 = vkCreateSemaphore(m_renderer.m_backend.m_device, &semaphoreInfo, nullptr, &m_imageAvailSemaphore);

	if (res1 != VK_SUCCESS && res2 != VK_SUCCESS) {
		throw std::runtime_error("could not create semaphore");
	}

	createCommandBuffer();
	recordCommandBuffers();
}

ClearScreenPass::~ClearScreenPass()
{
	vkDestroySemaphore(m_renderer.m_backend.m_device, m_renderWaitSemaphore, nullptr);
}

void ClearScreenPass::RenderFrame()
{
	uint32_t ImageIndex = 0;

	VkResult res = vkAcquireNextImageKHR(m_renderer.m_backend.m_device, m_renderer.m_backend.m_swapChain, UINT64_MAX, m_imageAvailSemaphore, nullptr, &ImageIndex);
	if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire next image");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_cmdBufs[ImageIndex];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_imageAvailSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_renderWaitSemaphore;

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;

	res = vkQueueSubmit(m_renderer.m_backend.m_presentQueue, 1, &submitInfo, nullptr);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to submit queue");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_renderer.m_backend.m_swapChain;
	presentInfo.pImageIndices = &ImageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderWaitSemaphore;

	res = vkQueuePresentKHR(m_renderer.m_backend.m_presentQueue, &presentInfo);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to present queue");
	}
}

void ClearScreenPass::createCommandBuffer()
{
	m_cmdBufs.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = m_renderer.m_backend.m_queueFamily.presentFamily.value();

	VkResult res = vkCreateCommandPool(m_renderer.m_backend.m_device, &createInfo, nullptr, &m_commandPool);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.commandPool = m_commandPool;
	cmdBufAllocInfo.commandBufferCount = m_renderer.m_backend.m_swapChainParams.swapChainImages.size();
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	res = vkAllocateCommandBuffers(m_renderer.m_backend.m_device, &cmdBufAllocInfo, m_cmdBufs.data());
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void ClearScreenPass::recordCommandBuffers()
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkImageSubresourceRange imageRange{};
	imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageRange.levelCount = 1;
	imageRange.layerCount = 1;
	imageRange.baseMipLevel = 0;
	imageRange.baseArrayLayer = 0;

	for (size_t i = 0; i < m_renderer.m_backend.m_swapChainParams.swapChainImages.size(); ++i)
	{
		VkImageMemoryBarrier presentToClearBarrier = {};
		presentToClearBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		presentToClearBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		presentToClearBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		presentToClearBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		presentToClearBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		presentToClearBarrier.srcQueueFamilyIndex = m_renderer.m_backend.m_queueFamily.presentFamily.value();
		presentToClearBarrier.dstQueueFamilyIndex = m_renderer.m_backend.m_queueFamily.presentFamily.value();
		presentToClearBarrier.image = m_renderer.m_backend.m_swapChainParams.swapChainImages[i];
		presentToClearBarrier.subresourceRange = imageRange;

		// Change layout of image to be optimal for presenting
		VkImageMemoryBarrier clearToPresentBarrier = {};
		clearToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		clearToPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		clearToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		clearToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		clearToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		clearToPresentBarrier.srcQueueFamilyIndex = m_renderer.m_backend.m_queueFamily.presentFamily.value();
		clearToPresentBarrier.dstQueueFamilyIndex = m_renderer.m_backend.m_queueFamily.presentFamily.value();
		clearToPresentBarrier.image = m_renderer.m_backend.m_swapChainParams.swapChainImages[i];
		clearToPresentBarrier.subresourceRange = imageRange;


		VkResult res = vkBeginCommandBuffer(m_cmdBufs[i], &beginInfo);
		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to begin command buffer");
		}

		vkCmdPipelineBarrier(m_cmdBufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToClearBarrier);

		vkCmdClearColorImage(m_cmdBufs[i], m_renderer.m_backend.m_swapChainParams.swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &m_clearColor, 1, &imageRange);

		vkCmdPipelineBarrier(m_cmdBufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &clearToPresentBarrier);

		res = vkEndCommandBuffer(m_cmdBufs[i]);
		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to end command buffer");
		}

	}

}