#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physical_device, VkDevice device, 
		VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
		std::vector<Vertex>* vertices,
		std::vector<u32>* indices);

	int getVertexCount();
	VkBuffer getVertexBuffer();

	int getIndexCount();
	VkBuffer getIndexBuffer();
	void destroyBuffers();
	~Mesh();
private:
	int vertex_count;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;

	int index_count;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;

	VkPhysicalDevice physical_device;
	VkDevice device;

	void createVertexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool, std::vector<Vertex>* vertices);
	void createIndexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool, std::vector<u32>* indices);

};

