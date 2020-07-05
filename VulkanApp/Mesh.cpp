#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice new_physical_device, VkDevice new_device, 
	VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
	std::vector<Vertex>* vertices, std::vector<u32>* indices) {
	vertex_count = vertices->size();
	index_count = indices->size();
	physical_device = new_physical_device;
	device = new_device;
	createVertexBuffer(transfer_queue, transfer_cmd_pool, vertices);
	createIndexBuffer(transfer_queue, transfer_cmd_pool, indices);

	model.model = glm::mat4(1.0f);
}

int Mesh::getVertexCount()
{
	return vertex_count;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertex_buffer;
}

int Mesh::getIndexCount()
{
	return index_count;
}

VkBuffer Mesh::getIndexBuffer()
{
	return index_buffer;
}

void Mesh::destroyBuffers() {
	vkDestroyBuffer(device, vertex_buffer, nullptr);
	vkFreeMemory(device, vertex_buffer_memory, nullptr);
	vkDestroyBuffer(device, index_buffer, nullptr);
	vkFreeMemory(device, index_buffer_memory, nullptr);
}

void Mesh::setModel(glm::mat4 new_model) {
	model.model = new_model;
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool, std::vector<Vertex>* vertices) {

	VkDeviceSize buffer_size = ((size_t)sizeof(Vertex)) * vertices->size();

	// staging buffer to "stage" vertex data before transferring to GPU
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;


	createBuffer(physical_device, device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&staging_buffer, &staging_buffer_memory);

	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices->data(), size_t(buffer_size));
	vkUnmapMemory(device, staging_buffer_memory);


	// TRANSFER_DST bit to trasnfer to GPU, as vertex buffer.
	// memory is local only to the GPU
	createBuffer(physical_device, device, buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&vertex_buffer, &vertex_buffer_memory);

	copyBuffer(device, transfer_queue, transfer_cmd_pool, staging_buffer, vertex_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool, std::vector<u32>* indices) {
	VkDeviceSize buffer_size = ((size_t)sizeof(u32)) * indices->size();

	// staging buffer to "stage" vertex data before transferring to GPU
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;


	createBuffer(physical_device, device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&staging_buffer, &staging_buffer_memory);

	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, indices->data(), size_t(buffer_size));
	vkUnmapMemory(device, staging_buffer_memory);

	createBuffer(physical_device, device, buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&index_buffer, &index_buffer_memory);

	copyBuffer(device, transfer_queue, transfer_cmd_pool, staging_buffer, index_buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);
}

