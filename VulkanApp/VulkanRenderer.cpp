#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)  {
	window = newWindow;

	try {
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error &e) {
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::cleanup() {
	vkDestroyInstance(instance, nullptr);
	vkDestroyDevice(mainDevice.logical_device, nullptr);
}

void VulkanRenderer::getPhysicalDevice() {
	uint32_t device_cnt = 0;
	vkEnumeratePhysicalDevices(instance, &device_cnt, nullptr);

	if (device_cnt == 0) {
		throw std::runtime_error("Can't find device that supports vulkan");
	}

	std::vector<VkPhysicalDevice> device_list(device_cnt);
	vkEnumeratePhysicalDevices(instance, &device_cnt, device_list.data());
	for (const auto& device : device_list) {
		if (checkDeviceSuitable(device)) {
			mainDevice.physical_device = device;
			break;
		}
	}
}

void VulkanRenderer::createInstance() {
	// Information about application itself
	// Mostly for dev convenience, debugging
	VkApplicationInfo appinfo = {};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "Vulkan App";
	appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.pEngineName = "NO ENGINE";
	appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	// Creation info for VK Instance
	VkInstanceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appinfo;

	// create list of instance extensions
	std::vector<const char*> instanceExtensions;
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	for (size_t i = 0; i < glfwExtensionsCount; i++) {
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	if (!checkInstanceExtensionsSupport(&instanceExtensions)) {
		throw std::runtime_error("Vk Instance does not support required extensions :(");

	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: Set up validation layers.
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan instance");
	}

}

void VulkanRenderer::createLogicalDevice() {
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physical_device);

	VkDeviceQueueCreateInfo queue_create_info = {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = indices.graphics_family;
	queue_create_info.queueCount = 1;
	float priority = 1.0f;
	queue_create_info.pQueuePriorities = &priority;


	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &queue_create_info;
	device_create_info.enabledExtensionCount = 0;			//Logical device extensions
	device_create_info.ppEnabledExtensionNames = nullptr;

	VkPhysicalDeviceFeatures device_features = {};
	device_create_info.pEnabledFeatures = &device_features;

	VkResult res = vkCreateDevice(mainDevice.physical_device, &device_create_info, nullptr, &mainDevice.logical_device);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device");
	}

	// Queues created with device
	vkGetDeviceQueue(mainDevice.logical_device, indices.graphics_family, 0, &graphics_queue);
}

bool VulkanRenderer::checkInstanceExtensionsSupport(std::vector<const char*>* checkExtenstions) {
	
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	
	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

	//check if given extension in availbalbe extensions

	for (const auto& checkExtenstion : *checkExtenstions) {
		bool hasExt = false;
		for (const auto& extension : extensions) {
			if (strcmp(checkExtenstion, extension.extensionName)) {
				hasExt = true;
				break;
			}
		}
		if (!hasExt) {
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device) {
	// VkPhysicalDeviceFeatures device_features;
	// vkGetPhysicalDeviceFeatures(device, &device_features);

	QueueFamilyIndices indices = getQueueFamilies(device);

	return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queue_family_cnt = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_cnt, nullptr);

	std::vector<VkQueueFamilyProperties> queue_family_list(queue_family_cnt);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_cnt, queue_family_list.data());
	int i = 0;
	for (const auto& family : queue_family_list) {
		if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}
		if (indices.isValid()) {
			break;
		}

		i++;
	}

	return indices;
}
