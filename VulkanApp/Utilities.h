#pragma once

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
