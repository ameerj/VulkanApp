#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include "Utilities.h"

class VulkanRenderer {
public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();

private:
	GLFWwindow* window;

	VkInstance instance;
	struct {
		VkPhysicalDevice physical_device;
		VkDevice logical_device;
	} mainDevice;
	VkQueue graphics_queue;

	// Vulkan functions

	void getPhysicalDevice();

	// - Create funcs
	void createInstance();
	void createLogicalDevice();

	// - Support funcs
	bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtenstions);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
};

