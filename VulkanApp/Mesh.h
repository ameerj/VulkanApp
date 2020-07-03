#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physical_device, VkDevice device, std::vector<Vertex>* vertices);

	int getVertexCount();
	VkBuffer getVertexBuffer();
	void destroyVertexBuffer();
	~Mesh();
private:
	int vertex_count;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;

	VkPhysicalDevice physical_device;
	VkDevice device;

	void createVertexBuffer(std::vector<Vertex>* vertices);

	uint32_t findMemoryTypeIndex(uint32_t allowed_types, VkMemoryPropertyFlags properties);
};

