#include "ScreenQuadRenderPass.h"
#include "Renderer.h"
#include <stdexcept>
#include "AssetUtilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace utils;

ScreenQuadRenderPass::ScreenQuadRenderPass(Vulkan_Renderer& renderer) : m_renderer{ renderer }
{
	model_path = "models/cornell_closed/cornell_closed.obj";

	createSemaphores();
	createRenderPass();
	createDescriptorLayout();
	createPipeline();	
	createFramebuffers();

	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createImageDescriptorLayout();
	createDescriptorLayout();	

	loadAssets();
}

ScreenQuadRenderPass::~ScreenQuadRenderPass()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_renderer.m_backend.m_device, waitImageAvailable[i], nullptr);
		vkDestroySemaphore(m_renderer.m_backend.m_device, waitRenderFinished[i], nullptr);
		vkDestroyFence(m_renderer.m_backend.m_device, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_renderer.m_backend.m_device, m_renderer.m_backend.m_commandPool, nullptr);

	for (auto framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_renderer.m_backend.m_device, framebuffer, nullptr);
	}

	vkDestroyDescriptorSetLayout(m_renderer.m_backend.m_device, m_descriptorSetLayout, nullptr);

	vkDestroyPipeline(m_renderer.m_backend.m_device, m_ScreenQuadPipeline, nullptr);
	vkDestroyPipelineLayout(m_renderer.m_backend.m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_renderer.m_backend.m_device, m_renderPass, nullptr);
}

void ScreenQuadRenderPass::RenderFrame()
{
	vkWaitForFences(m_renderer.m_backend.m_device, 1, &m_inFlightFences[m_renderer.currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_renderer.m_backend.m_device, m_renderer.m_backend.m_swapChain, UINT64_MAX, waitImageAvailable[m_renderer.currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_renderer.m_backend.m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[imageIndex] = m_inFlightFences[m_renderer.currentFrame];

	updateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { waitImageAvailable[m_renderer.currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { waitRenderFinished[m_renderer.currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_renderer.m_backend.m_device, 1, &m_inFlightFences[m_renderer.currentFrame]);
	if (vkQueueSubmit(m_renderer.m_backend.m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_renderer.currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_renderer.m_backend.m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;
	vkQueuePresentKHR(m_renderer.m_backend.m_presentQueue, &presentInfo);

	m_renderer.currentFrame = (m_renderer.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void ScreenQuadRenderPass::createSemaphores()
{
	waitImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	waitRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size(), VK_NULL_HANDLE);


	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_renderer.m_backend.m_device, &semaphoreInfo, nullptr, &waitImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_renderer.m_backend.m_device, &semaphoreInfo, nullptr, &waitRenderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(m_renderer.m_backend.m_device, &fenceInfo, nullptr, &m_inFlightFences[i])) {

			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void ScreenQuadRenderPass::createImageDescriptorLayout()
{
	//add other samplers
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding binding = samplerLayoutBinding;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_renderer.m_backend.m_device
		, &layoutInfo, nullptr, &m_imageDescriptorSetLayout),
		"failed to create descriptor set layout");
}


void ScreenQuadRenderPass::createDescriptorLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutBinding binding = uboLayoutBinding;
	
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_renderer.m_backend.m_device
	,&layoutInfo, nullptr, &m_descriptorSetLayout),
		"failed to create descriptor set layout");
}

void ScreenQuadRenderPass::updateUniformBuffer(uint32_t index) 
{		
	
	globalShaderVars vars;
	vars.totalElapsedTime = m_renderer.elapsedTime;
	vars.frameTime = m_renderer.frameTime;
	
	void* data;
	vkMapMemory(m_renderer.m_backend.m_device, m_uniformBuffersMemory[index], 0, sizeof(vars), 0, &data);
	memcpy(data, &vars, sizeof(vars));
	vkUnmapMemory(m_renderer.m_backend.m_device, m_uniformBuffersMemory[index]);
}

void ScreenQuadRenderPass::createDescriptorPool()
{
	auto size = m_renderer.m_backend.m_swapChainParams.swapChainImages.size();

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(size);

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(size);


	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(size);

	VK_CHECK_RESULT(vkCreateDescriptorPool(m_renderer.m_backend.m_device, &poolInfo, nullptr,  &m_renderer.m_backend.m_descriptorPool), "failed to create descriptor pool");

}

//create UBO descriptorSets
void ScreenQuadRenderPass::createDescriptorSets()
{
	//UBO Descriptor set
	std::vector<VkDescriptorSetLayout> layout(m_renderer.m_backend.m_swapChainParams.swapChainImages.size(), m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_renderer.m_backend.m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());
	allocInfo.pSetLayouts = layout.data();

	m_descriptorSets.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());
	VK_CHECK_RESULT(vkAllocateDescriptorSets(m_renderer.m_backend.m_device,
		&allocInfo,
		m_descriptorSets.data())
	,"failed to create descriptor sets");

	for (size_t i = 0; i < m_renderer.m_backend.m_swapChainParams.swapChainImages.size(); ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(globalShaderVars);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_renderer.m_backend.m_device, 1,  &descriptorWrite, 0, nullptr);
	}


}

void ScreenQuadRenderPass::createImageDescriptorSets()
{
	//UBO Descriptor set
	std::vector<VkDescriptorSetLayout> layout(m_renderer.m_backend.m_swapChainParams.swapChainImages.size(), m_imageDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_renderer.m_backend.m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());
	allocInfo.pSetLayouts = layout.data();

	m_ImageDescriptorSets.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());
	VK_CHECK_RESULT(vkAllocateDescriptorSets(m_renderer.m_backend.m_device,
		&allocInfo,
		m_ImageDescriptorSets.data())
		, "failed to create descriptor sets");

	for (auto& m : m_meshList)
	{
		for (size_t i = 0; i < m_renderer.m_backend.m_swapChainParams.swapChainImages.size(); ++i)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_ImageDescriptorSets[i];
			descriptorWrite.dstBinding = 1;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &m.mat.diffuse->imgDescriptor;				

			vkUpdateDescriptorSets(m_renderer.m_backend.m_device, 1, &descriptorWrite, 0, nullptr);
		}
	}

}

void ScreenQuadRenderPass::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(globalShaderVars);

	m_uniformBuffers.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());
	m_uniformBuffersMemory.resize(m_renderer.m_backend.m_swapChainParams.swapChainImages.size());

	for (size_t i = 0; i < m_renderer.m_backend.m_swapChainParams.swapChainImages.size(); ++i)
	{
		createBuffer(m_renderer.m_backend, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_uniformBuffers[i], m_uniformBuffersMemory[i]
		);
	}
}

//fullscreen quad
void ScreenQuadRenderPass::createPipeline()
{
	auto vertShaderCode = readFile("shaders/fsQuadvs.spv");
	auto fragShaderCode = readFile("shaders/fsQuadfs.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, m_renderer.m_backend.m_device);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, m_renderer.m_backend.m_device);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo , fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; 
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	m_renderer.m_viewport.x = 0.0f;
	m_renderer.m_viewport.y = 0.0f;
	m_renderer.m_viewport.width = (float)m_renderer.m_backend.m_swapChainParams.swapChainExtent.width;
	m_renderer.m_viewport.height = (float)m_renderer.m_backend.m_swapChainParams.swapChainExtent.height;
	m_renderer.m_viewport.minDepth = 0.0f;
	m_renderer.m_viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { m_renderer.m_backend.m_swapChainParams.swapChainExtent };

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &m_renderer.m_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

	if (vkCreatePipelineLayout(m_renderer.m_backend.m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; //TODO:Add
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	

	if (
		vkCreateGraphicsPipelines(m_renderer.m_backend.m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_ScreenQuadPipeline) != VK_SUCCESS
		)
	{
		throw std::runtime_error("failed to create full screen quad pipeline!");
	}

	vkDestroyShaderModule(m_renderer.m_backend.m_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_renderer.m_backend.m_device, vertShaderModule, nullptr);
}

void ScreenQuadRenderPass::createFramebuffers()
{
	m_swapChainFramebuffers.resize(m_renderer.m_backend.m_swapChainParams.swapChainImageViews.size());

	for (size_t i = 0; i < m_renderer.m_backend.m_swapChainParams.swapChainImageViews.size(); ++i) {
		VkImageView attachments[] = { m_renderer.m_backend.m_swapChainParams.swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_renderer.m_backend.m_swapChainParams.swapChainExtent.width;
		framebufferInfo.height = m_renderer.m_backend.m_swapChainParams.swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (
			vkCreateFramebuffer(m_renderer.m_backend.m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i])
			)
		{
			throw std::runtime_error("Failed to create Framebuffer");
		}
	}
}

void ScreenQuadRenderPass::freeResources()
{
	vkDeviceWaitIdle(m_renderer.m_backend.m_device);

	for (size_t i = 0; i < m_swapChainFramebuffers.size(); ++i)
	{
		vkDestroyBuffer(m_renderer.m_backend.m_device, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_renderer.m_backend.m_device, m_uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_renderer.m_backend.m_device, m_renderer.m_backend.m_descriptorPool, nullptr);

	for (size_t i = 0; i < m_swapChainFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(m_renderer.m_backend.m_device, m_swapChainFramebuffers[i], nullptr);
	}

	vkFreeCommandBuffers(m_renderer.m_backend.m_device, m_renderer.m_backend.m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
	vkDestroyPipeline(m_renderer.m_backend.m_device, m_ScreenQuadPipeline, nullptr);
	vkDestroyPipelineLayout(m_renderer.m_backend.m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_renderer.m_backend.m_device, m_renderPass, nullptr);
}

void ScreenQuadRenderPass::recreateResources()
{
	createRenderPass();
	createDescriptorLayout();
	createPipeline();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void ScreenQuadRenderPass::loadAssets()
{
	utils::loadOBJ(model_path, m_renderer.m_backend);

	for (auto& m : m_meshList)
	{
		if (m.mat.diffuse != nullptr)
		{
			m.mat.CreateMaterial(m_renderer.m_backend, m_renderer.m_backend.m_descriptorPool, m_imageDescriptorSetLayout);
		}		
	}
}

void ScreenQuadRenderPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_renderer.m_backend.m_swapChainParams.swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPass{};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPass;


	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;


	VkResult res = vkCreateRenderPass(m_renderer.m_backend.m_device, &renderPassInfo, nullptr, &m_renderPass);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

//rerecord and submit should be per frame
void ScreenQuadRenderPass::createCommandBuffers()
{
	m_commandBuffers.resize(m_swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_renderer.m_backend.m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_swapChainFramebuffers.size());

	if (vkAllocateCommandBuffers(m_renderer.m_backend.m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers");
	}

	VkResult res;
	for (size_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		res = vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);
		if (res != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_renderer.m_backend.m_swapChainParams.swapChainExtent;
		VkClearValue clearColor = { 1.0f, 0.8f, 0.0f, .0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_ScreenQuadPipeline);

		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);
		vkCmdDraw(m_commandBuffers[i], 4, 1, 0, 0);
		
		

		vkCmdEndRenderPass(m_commandBuffers[i]);

		res = vkEndCommandBuffer(m_commandBuffers[i]);
		if (res != VK_SUCCESS) throw std::runtime_error("failed to end recording command buffer");
	}


}



