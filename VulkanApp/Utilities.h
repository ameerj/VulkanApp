#pragma once

#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static bool VKCheckError(VkResult res, const char* file, int line) {
	if (res != VK_SUCCESS) {
		std::cout << "[Vulkan Error] (" << res << "): " <<
			file << ":" << line << std::endl;
		return false;
	}
	return true;
}

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

#define ASSERT(x) if (!(x)) __debugbreak();
#define VKRes(x)  ASSERT(VKCheckError(x, __FILE__, __LINE__))

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAME_DRAWS = 2;

struct Vertex {
	glm::vec3 pos;	//position xyz
	glm::vec3 col;	//color rgb
};

// Indices of Queue families (if they exist)
struct QueueFamilyIndices {
	int graphics_family = -1;
	int presentation_family = -1;

	bool isValid() {
		return graphics_family >= 0 && presentation_family >= 0;
	}
};


struct SwapchainDetails {
	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentation_modes;
};

struct SwapchainImage {
	VkImage image;
	VkImageView image_view;
};

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	ASSERT(file.is_open());
	
	size_t file_size = (size_t)file.tellg();
	std::vector<char> file_buffer(file_size);

	file.seekg(0);
	file.read(file_buffer.data(), file_size);

	file.close();
	return file_buffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physcial_device, uint32_t allowed_types, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memprops;
	vkGetPhysicalDeviceMemoryProperties(physcial_device, &memprops);

	for (u32 i = 0; i < memprops.memoryTypeCount; i++) {
		if ((allowed_types & (1 << i))
			&& (memprops.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
}

static void createBuffer(VkPhysicalDevice physcial_device, VkDevice device, VkDeviceSize buffer_size,
	VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* buffermem) {
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = buffer_size;
	buffer_info.usage = buffer_usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VKRes(vkCreateBuffer(device, &buffer_info, nullptr, buffer));

	// Get buffer memory reqs
	VkMemoryRequirements memreqs;
	vkGetBufferMemoryRequirements(device, *buffer, &memreqs);

	VkMemoryAllocateInfo memalloc_info = {};
	memalloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memalloc_info.allocationSize = memreqs.size;
	memalloc_info.memoryTypeIndex = findMemoryTypeIndex(physcial_device, memreqs.memoryTypeBits, properties);

	VKRes(vkAllocateMemory(device, &memalloc_info, nullptr, buffermem));

	vkBindBufferMemory(device, *buffer, *buffermem, 0);

}

static void copyBuffer(VkDevice device, VkQueue transfer_queue, 
	VkCommandPool transfer_cmd_pool, VkBuffer src, VkBuffer dst, VkDeviceSize buffer_size) {
	VkCommandBuffer transfer_cmd_buffer;

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = transfer_cmd_pool;
	alloc_info.commandBufferCount = 1;

	vkAllocateCommandBuffers(device, &alloc_info, &transfer_cmd_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(transfer_cmd_buffer, &begin_info);

	VkBufferCopy buffer_copy_region = {};
	buffer_copy_region.srcOffset = 0;
	buffer_copy_region.dstOffset = 0;
	buffer_copy_region.size = buffer_size;

	vkCmdCopyBuffer(transfer_cmd_buffer, src, dst, 1, &buffer_copy_region);
	vkEndCommandBuffer(transfer_cmd_buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &transfer_cmd_buffer;

	vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, transfer_cmd_pool, 1, &transfer_cmd_buffer);
}
