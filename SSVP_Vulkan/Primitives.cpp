#include "Primitives.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 
#include <iostream>
#include <stdexcept>

VkVertexInputBindingDescription getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(vertex, uv);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(vertex, normal);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(vertex, tangent);

	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(vertex, bitangent);

	return attributeDescriptions;
}


void Texture::setupTexture(Vulkan_Backend& backend)
{
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		
	if (!pixels)
	{
		std::cout << "Texture failed to load at path: " << this->path.c_str() << std::endl;
		std::cout << stbi_failure_reason() << std::endl;
		stbi_image_free(pixels);
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(backend, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(backend.m_device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(backend.m_device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(backend, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, img, imgMem);

	transitionImageLayout(backend, img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copyBufferToImage(backend, stagingBuffer,img, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	transitionImageLayout(backend, img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(backend.m_device, stagingBuffer, nullptr);
	vkFreeMemory(backend.m_device, stagingBufferMemory, nullptr);

	imgView = createImageView(backend, img, VK_FORMAT_R8G8B8A8_SRGB);

	//create sampler

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(backend.m_physicalDevice, &properties);

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	auto res = vkCreateSampler(backend.m_device, &samplerInfo, nullptr, &imgSampler);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create  sampler");
	}

	imgDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgDescriptor.imageView = imgView;
	imgDescriptor.sampler = imgSampler;


}

void Material::CreateMaterial(Vulkan_Backend& backend, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocateInfo{};	

	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = descriptorPool;
	allocateInfo.pSetLayouts = &descriptorSetLayout;
	allocateInfo.descriptorSetCount = 1;

	auto res = vkAllocateDescriptorSets(backend.m_device,
		&allocateInfo,
		&matDescriptorSet
		);

	if (res != VK_SUCCESS) throw std::runtime_error("failed to create material descriptor set");

	std::vector<VkDescriptorImageInfo> imageDescriptors{};
	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

	imageDescriptors.push_back(diffuse->imgDescriptor);
	
	VkWriteDescriptorSet writeDescSet{};
	writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescSet.descriptorCount = 1;
	writeDescSet.dstSet = matDescriptorSet;
	writeDescSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
	writeDescSet.pImageInfo = &diffuse->imgDescriptor;
	
	writeDescriptorSets.push_back(writeDescSet);

	vkUpdateDescriptorSets(backend.m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void Mesh::SetupMesh()
{

}
