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
		createDebugMessenger();

		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createGraphicsPipeline();
	}
	catch (const std::runtime_error &e) {
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::cleanup() {
	for (auto img : sc_images) {
		vkDestroyImageView(mainDevice.logical_device, img.image_view, nullptr);
	}
	vkDestroySwapchainKHR(mainDevice.logical_device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logical_device, nullptr);
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
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

	VKRes(vkCreateInstance(&createInfo, nullptr, &instance));


}

void VulkanRenderer::createLogicalDevice() {
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<int> queue_family_indices = { indices.graphics_family, indices.presentation_family };

	for (int index : queue_family_indices) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = index;
		queue_create_info.queueCount = 1;
		float priority = 1.0f;
		queue_create_info.pQueuePriorities = &priority;

		queue_create_infos.push_back(queue_create_info);
	}

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_create_info.pQueueCreateInfos = queue_create_infos.data();
	device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());			//Logical device extensions
	device_create_info.ppEnabledExtensionNames = device_extensions.data();

	VkPhysicalDeviceFeatures device_features = {};
	device_create_info.pEnabledFeatures = &device_features;

	VKRes(vkCreateDevice(mainDevice.physical_device, &device_create_info, nullptr, &mainDevice.logical_device));



	// Queues created with device
	vkGetDeviceQueue(mainDevice.logical_device, indices.graphics_family, 0, &graphics_queue);
	vkGetDeviceQueue(mainDevice.logical_device, indices.presentation_family, 0, &presentation_queue);
}

void VulkanRenderer::createDebugMessenger() {
	if (!enableValidationLayers) {
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void VulkanRenderer::createSurface() {
	VKRes(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

void VulkanRenderer::createSwapchain() {
	SwapchainDetails swapchain_details = getSwapchainDetails(mainDevice.physical_device);

	// choose best surface, then pres mode, then image res
	VkSurfaceFormatKHR format = chooseBestFormat(swapchain_details.formats);
	VkPresentModeKHR mode = chooseBestPresMode(swapchain_details.presentation_modes);
	VkExtent2D extent = chooseSwapExtent(swapchain_details.surface_capabilities);

	uint32_t img_cnt = swapchain_details.surface_capabilities.minImageCount + 1;	// plus 1 for triple buffering
	if (swapchain_details.surface_capabilities.maxImageCount > 0 &&
		swapchain_details.surface_capabilities.maxImageCount < img_cnt) {
		img_cnt = swapchain_details.surface_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR sc_create_info = {};
	sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_create_info.surface = surface;
	sc_create_info.imageFormat = format.format;
	sc_create_info.imageColorSpace = format.colorSpace;
	sc_create_info.presentMode = mode;
	sc_create_info.imageExtent = extent;
	sc_create_info.minImageCount = img_cnt;
	sc_create_info.imageArrayLayers = 1;
	sc_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sc_create_info.preTransform = swapchain_details.surface_capabilities.currentTransform;
	sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sc_create_info.clipped = VK_TRUE;

	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physical_device);
	if (indices.graphics_family != indices.presentation_family) {
		uint32_t q_indices[] = {
			indices.graphics_family,
			indices.presentation_family
		};
		sc_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		sc_create_info.queueFamilyIndexCount = 2;
		sc_create_info.pQueueFamilyIndices = q_indices;
	}
	else {
		sc_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sc_create_info.queueFamilyIndexCount = 0;
		sc_create_info.pQueueFamilyIndices = nullptr;
	}
	sc_create_info.oldSwapchain = VK_NULL_HANDLE;

	VKRes(vkCreateSwapchainKHR(mainDevice.logical_device, &sc_create_info, nullptr, &swapchain));


	sc_img_format = format.format;
	sc_extent = extent;

	// get swapchain images
	uint32_t sc_img_cnt;
	vkGetSwapchainImagesKHR(mainDevice.logical_device, swapchain, &sc_img_cnt, nullptr);
	std::vector<VkImage> images(sc_img_cnt);
	vkGetSwapchainImagesKHR(mainDevice.logical_device, swapchain, &sc_img_cnt, images.data());

	for (VkImage img : images) {
		SwapchainImage sc_image = {};
		sc_image.image = img;

		sc_image.image_view = createIMageView(img, sc_img_format, VK_IMAGE_ASPECT_COLOR_BIT);
		sc_images.push_back(sc_image);
	}
}

void VulkanRenderer::createGraphicsPipeline() {
	auto vertex_shader_code = readFile("shaders/vert.spv");
	auto fragment_shader_code = readFile("shaders/frag.spv");

	VkShaderModule vertex_shader = createShaderModule(vertex_shader_code);
	VkShaderModule fragment_shader = createShaderModule(fragment_shader_code);

	VkPipelineShaderStageCreateInfo vertex_info = {};
	vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_info.module = vertex_shader;
	vertex_info.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_info = {};
	fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_info.module = fragment_shader;
	fragment_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_info, fragment_info };

	// create pipeline

	// destroy shader modules after pipeline has been created.
	vkDestroyShaderModule(mainDevice.logical_device, vertex_shader, nullptr);
	vkDestroyShaderModule(mainDevice.logical_device, fragment_shader, nullptr);
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
			if (strcmp(checkExtenstion, extension.extensionName) == 0) {
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

bool VulkanRenderer::checkDeviceInstanceExtensionsSupport(VkPhysicalDevice device) {
	uint32_t ext_cnt = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_cnt, nullptr);

	if (ext_cnt == 0) {
		return false;
	}

	std::vector<VkExtensionProperties> extensions(ext_cnt);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_cnt, extensions.data());

	for (const auto& dev_ext : device_extensions) {
		bool has_extension = false;
		for (const auto& ext : extensions) {
			if (strcmp(dev_ext, ext.extensionName) == 0) {
				has_extension = true;
				break;
			}
		}
		if (!has_extension) {
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device) {
	// VkPhysicalDeviceFeatures device_features;
	// vkGetPhysicalDeviceFeatures(device, &device_features);

	QueueFamilyIndices indices = getQueueFamilies(device);

	bool extensions_supported = checkDeviceInstanceExtensionsSupport(device);

	bool swapcahin_valid = false;
	if (extensions_supported) {
		SwapchainDetails details = getSwapchainDetails(device);
		swapcahin_valid = !details.presentation_modes.empty() && !details.formats.empty();
	}
	return indices.isValid() && extensions_supported && swapcahin_valid;
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

		VkBool32 presentation_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support);

		if (family.queueCount > 0 && presentation_support) {
			indices.presentation_family = i;
		}

		if (indices.isValid()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device) {
	SwapchainDetails swapchain_details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchain_details.surface_capabilities);

	uint32_t format_cnt = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_cnt, nullptr);
	if (format_cnt) {
		swapchain_details.formats.resize(format_cnt);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_cnt, swapchain_details.formats.data());
	}

	uint32_t presentation_cnt = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_cnt, nullptr);
	if (presentation_cnt) {
		swapchain_details.presentation_modes.resize(presentation_cnt);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_cnt, swapchain_details.presentation_modes.data());
	}

	return swapchain_details;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresMode(const std::vector<VkPresentModeKHR>& modes) {
	for (const auto& mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D new_extent = {};
		new_extent.width = static_cast<uint32_t>(width);
		new_extent.height = static_cast<uint32_t>(height);

		new_extent.width = std::min(capabilities.maxImageExtent.width, new_extent.width);
		new_extent.width = std::max(capabilities.minImageExtent.width, new_extent.width);

		new_extent.height = std::min(capabilities.maxImageExtent.height, new_extent.height);
		new_extent.height = std::max(capabilities.minImageExtent.height, new_extent.height);

		return new_extent;
	}
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

VkImageView VulkanRenderer::createIMageView(VkImage image, VkFormat format, VkImageAspectFlags flags) {
	VkImageViewCreateInfo view_info = {};

	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	
	view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	view_info.subresourceRange.aspectMask = flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView img_view;
	VKRes(vkCreateImageView(mainDevice.logical_device, &view_info, nullptr, &img_view));
	return img_view;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo shader_info = {};
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = code.size();
	shader_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	VKRes(vkCreateShaderModule(mainDevice.logical_device, &shader_info, nullptr, &shader_module));
	
	return shader_module;
}
