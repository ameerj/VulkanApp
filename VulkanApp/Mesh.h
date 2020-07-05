#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

struct UboModel {
	glm::mat4 model;
};

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

	void setModel(glm::mat4 new_model);
	UboModel getModel() { return model; }

	~Mesh();
private:
	UboModel model;
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

