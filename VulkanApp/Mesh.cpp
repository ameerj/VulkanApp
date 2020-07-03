#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice new_physical_device, VkDevice new_device, std::vector<Vertex>* vertices) {
	vertex_count = vertices->size();
	physical_device = new_physical_device;
	device = new_device;
	createVertexBuffer(vertices);
}

int Mesh::getVertexCount()
{
	return vertex_count;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertex_buffer;
}

void Mesh::destroyVertexBuffer() {
	vkDestroyBuffer(device, vertex_buffer, nullptr);
	vkFreeMemory(device, vertex_buffer_memory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices) {
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = sizeof(Vertex) * vertices->size();
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VKRes(vkCreateBuffer(device, &buffer_info, nullptr, &vertex_buffer));

	// Get buffer memory reqs
	VkMemoryRequirements memreqs;
	vkGetBufferMemoryRequirements(device, vertex_buffer, &memreqs);

	VkMemoryAllocateInfo memalloc_info = {};
	memalloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memalloc_info.allocationSize = memreqs.size;
	memalloc_info.memoryTypeIndex = findMemoryTypeIndex(memreqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VKRes(vkAllocateMemory(device, &memalloc_info, nullptr, &vertex_buffer_memory));

	vkBindBufferMemory(device, vertex_buffer, vertex_buffer_memory, 0);

	void* data;
	vkMapMemory(device, vertex_buffer_memory, 0, buffer_info.size, 0, &data);
	memcpy(data, vertices->data(), size_t(buffer_info.size));
	vkUnmapMemory(device, vertex_buffer_memory);

}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowed_types, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memprops;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memprops);

	for (u32 i = 0; i < memprops.memoryTypeCount; i++) {
		if ((allowed_types & (1 << i))
			&& (memprops.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
}
