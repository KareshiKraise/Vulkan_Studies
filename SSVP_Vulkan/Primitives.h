#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include <array>
#include <string>
#include "Renderer.h"

struct globalShaderVars {
	float totalElapsedTime;
	float frameTime;
};

struct vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;	
};

struct Texture {
	VkSampler imgSampler;
	VkImage img;
	VkImageView imgView;
	VkDeviceMemory imgMem;
	VkDescriptorImageInfo imgDescriptor;
	VkImageLayout imgLayout;
	std::string type;
	std::string path;

	void setupTexture(Vulkan_Backend& backend);
};

struct Material {
	Texture* diffuse;
	VkDescriptorSet matDescriptorSet = VK_NULL_HANDLE;

	//create material, descriptor set
	void CreateMaterial(Vulkan_Backend& backend, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
};

VkVertexInputBindingDescription getBindingDescription();

std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();

struct Mesh {
	//Mesh(std::vector<vertex> v, std::vector<uint16_t> i, std::vector<Texture> t){}
	Mesh(std::vector<vertex> v, std::vector<uint16_t> i, Material m) { vertices = v; indices = i; mat = m; };
	std::vector<vertex> vertices;
	std::vector<uint16_t> indices;
	Material mat;
	//std::vector<Texture> textures;
};
