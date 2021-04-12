#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "Renderer.h"
#include "Primitives.h"

class VertexBuffer {
public:
	VertexBuffer();
	VertexBuffer(std::vector<vertex> data, std::vector<uint16_t> ind, Vulkan_Backend& backend, VkCommandPool commandPool);
	
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;	

	void destroyVertexBuffer(Vulkan_Backend& backend);
	void createVertexBuffer(std::vector<vertex> data, Vulkan_Backend& backend, VkCommandPool commandPool);
	void createIndexBuffer(std::vector<uint16_t> data, Vulkan_Backend& backend, VkCommandPool commandPool);

		
};
