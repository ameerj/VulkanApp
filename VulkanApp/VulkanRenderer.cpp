#include <iostream>
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
		setupDebugMessenger();
	}
	catch (const std::runtime_error &e) {
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::cleanup() {
	
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
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
	std::vector<const char*> instanceExtensions = getRequiredExtensions();

	if (!checkInstanceExtensionsSupport(&instanceExtensions)) {
		throw std::runtime_error("Vk Instance does not support required extensions :(");

	}

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

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

void VulkanRenderer::setupDebugMessenger() {
	if (!enableValidationLayers) {
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
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

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device) {
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

bool VulkanRenderer::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
