#pragma once

#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

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
