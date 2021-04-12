#include "VertexBuffer.h"
#include <stdexcept>
#include "Renderer.h"

//INSIGHT: This should belong in a backend perhaps?
VertexBuffer::VertexBuffer()
{
}

VertexBuffer::VertexBuffer(std::vector<vertex> data, std::vector<uint16_t> ind, Vulkan_Backend& backend, VkCommandPool commandPool)
{
	createVertexBuffer(data, backend,commandPool);
	createIndexBuffer(  ind   ,backend, commandPool);
}

void VertexBuffer::destroyVertexBuffer(Vulkan_Backend& backend)
{
	vkDestroyBuffer(backend.m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(backend.m_device, m_vertexBufferMemory, nullptr);
}

void VertexBuffer::createVertexBuffer(std::vector<vertex> data, Vulkan_Backend& backend, VkCommandPool commandPool)
{
	VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(backend, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* memData;
	vkMapMemory(backend.m_device, m_vertexBufferMemory, 0, bufferSize, 0, &memData);
	memcpy(memData, data.data(), (size_t)bufferSize);
	vkUnmapMemory(backend.m_device, m_vertexBufferMemory);

	createBuffer(backend, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

	copyBuffer(backend, stagingBuffer, m_vertexBuffer, bufferSize, commandPool);


	vkDestroyBuffer(backend.m_device, stagingBuffer, nullptr);
	vkFreeMemory(backend.m_device, stagingBufferMemory, nullptr);
}

void VertexBuffer::createIndexBuffer(std::vector<uint16_t> data, Vulkan_Backend& backend, VkCommandPool commandPool) {
	VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(backend, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* memData;
	vkMapMemory(backend.m_device, stagingBufferMemory, 0, bufferSize, 0, &memData);
	memcpy(memData, data.data(), (size_t)bufferSize);
	vkUnmapMemory(backend.m_device, stagingBufferMemory);

	createBuffer(backend, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	copyBuffer(backend, stagingBuffer, m_indexBuffer, bufferSize, commandPool);

	vkDestroyBuffer(backend.m_device, stagingBuffer, nullptr);
	vkFreeMemory(backend.m_device, stagingBufferMemory, nullptr);
}


