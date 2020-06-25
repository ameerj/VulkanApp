#pragma once

#include <fstream>

static bool VKCheckError(VkResult res, const char* file, int line) {
	if (res != VK_SUCCESS) {
		std::cout << "[Vulkan Error] (" << res << "): " <<
			file << ":" << line << std::endl;
		return false;
	}
	return true;
}

#define ASSERT(x) if (!(x)) __debugbreak();
#define VKRes(x)  ASSERT(VKCheckError(x, __FILE__, __LINE__))

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
