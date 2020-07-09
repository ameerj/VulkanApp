#include <iostream>
#include "VulkanRenderer.h"
#include <array>

VulkanRenderer::VulkanRenderer(){}

VulkanRenderer::~VulkanRenderer(){}

int VulkanRenderer::init(GLFWwindow* newWindow)  {
	window = newWindow;

	try {
		createInstance();
		createDebugMessenger();

		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createRenderPass();
		createDescriptorSetLayout();

		createPushConstantRange();

		createGraphicsPipeline();

		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		//allocateDynamicBufferTransferSpace();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();

		ubo_view_proj.projection = glm::perspective(glm::radians(45.0f), (float)sc_extent.width / (float)sc_extent.height, 
			0.1f, 100.0f);
		ubo_view_proj.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		ubo_view_proj.projection[1][1] *= -1;

		std::vector<Vertex> mesh_verts = {
			{{ 0.0, -0.4, 0.0},  {1.0f, 0.0f, 0.0f}},
			{{ 0.0,  0.4, 0.0},  {0.0f, 1.0f, 0.0f}},
			{{-0.8,  0.4, 0.0},  {0.0f, 0.0f, 1.0f}},
			{{-0.8, -0.4, 0.0},  {0.0f, 1.0f, 0.0f}},
		};
		std::vector<Vertex> mesh_verts2 = {
			{{ 0.9, -0.4, 0.0},  {0.5f, 0.0f, 0.0f}},
			{{ 0.9,  0.4, 0.0},  {0.4f, 1.0f, 0.0f}},
			{{ 0.1,  0.4, 0.0},  {0.3f, 0.2f, 0.8f}},
			{{ 0.1, -0.4, 0.0},  {0.6f, 0.2f, 0.1f}},
		};
		// index data
		std::vector<u32> mesh_indices = {
			0, 1, 2,
			2, 3, 0,
		};

		Mesh first_mesh = Mesh(mainDevice.physical_device, mainDevice.logical_device,
			graphics_queue, graphics_command_pool, &mesh_verts, &mesh_indices);
		Mesh second_mesh = Mesh(mainDevice.physical_device, mainDevice.logical_device,
			graphics_queue, graphics_command_pool, &mesh_verts2, &mesh_indices);
		meshes.push_back(first_mesh);
		meshes.push_back(second_mesh);

		createSynchronisation();
	}
	catch (const std::runtime_error &e) {
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::updateModel(int id, glm::mat4 new_model) {
	//ubo_view_proj.model = new_model;
	if (id > meshes.size()) return;
	meshes[id].setModel(new_model);
}

void VulkanRenderer::draw() {
	// 1. get next available image, signal that it's ready to draw (semaphore)

	vkWaitForFences(mainDevice.logical_device, 1, &draw_fences[current_frame],
		VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(mainDevice.logical_device, 1, &draw_fences[current_frame]);

	uint32_t img_index;
	vkAcquireNextImageKHR(mainDevice.logical_device, swapchain, std::numeric_limits<uint64_t>::max(),
		image_available[current_frame], VK_NULL_HANDLE, &img_index);

	recordCommands(img_index);

	updateUniformBuffers(img_index);

	// 2. submit cmd buffer to queue for exec, waits for image to be signalled as available. signal when done
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &image_available[current_frame];

	VkPipelineStageFlags wait_stages[]{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[img_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &render_finished[current_frame];
	
	VKRes(vkQueueSubmit(graphics_queue, 1, &submit_info, draw_fences[current_frame]));

	// 3. present to screen
	VkPresentInfoKHR pres_info = {};
	pres_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pres_info.waitSemaphoreCount = 1;
	pres_info.pWaitSemaphores = &render_finished[current_frame];
	pres_info.swapchainCount = 1;
	pres_info.pSwapchains = &swapchain;
	pres_info.pImageIndices = &img_index;

	VKRes(vkQueuePresentKHR(presentation_queue, &pres_info));

	current_frame = (current_frame + 1) % MAX_FRAME_DRAWS;
}


void VulkanRenderer::cleanup() {

	vkDeviceWaitIdle(mainDevice.logical_device);

	//_aligned_free(model_transfer_space);

	vkDestroyDescriptorPool(mainDevice.logical_device, descriptor_pool, nullptr);

	vkDestroyDescriptorSetLayout(mainDevice.logical_device, descriptor_set_layout, nullptr);
	for (size_t i = 0; i < swapchain_images.size(); i++) {
		vkDestroyBuffer(mainDevice.logical_device, vp_uniform_buffer[i], nullptr);
		vkFreeMemory(mainDevice.logical_device, vp_uniform_buffer_memory[i], nullptr);
		// vkDestroyBuffer(mainDevice.logical_device, model_uniform_buffer[i], nullptr);
		// vkFreeMemory(mainDevice.logical_device, model_uniform_buffer_memory[i], nullptr);
	}
	for (auto mesh : meshes) {
		mesh.destroyBuffers();
	}


	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
		vkDestroySemaphore(mainDevice.logical_device, render_finished[i], nullptr);
		vkDestroySemaphore(mainDevice.logical_device, image_available[i], nullptr);
		vkDestroyFence(mainDevice.logical_device, draw_fences[i], nullptr);
	}
	vkDestroyCommandPool(mainDevice.logical_device, graphics_command_pool, nullptr);
	for (auto fb : swapchain_framebuffers) {
		vkDestroyFramebuffer(mainDevice.logical_device, fb, nullptr);
	}

	vkDestroyPipeline(mainDevice.logical_device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(mainDevice.logical_device, pipeline_layout, nullptr);
	vkDestroyRenderPass(mainDevice.logical_device, render_pass, nullptr);
	for (auto img : swapchain_images) {
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

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(mainDevice.physical_device, &props);

	// min_uniform_buff_offset = props.limits.minUniformBufferOffsetAlignment;
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
		swapchain_images.push_back(sc_image);
	}
}

void VulkanRenderer::createRenderPass() {
	// Colour attachment of render pass
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = sc_img_format;							// Format to use for attachment
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// Describes what to do with attachment after rendering
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to render_pass_info
	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Information about a particular subpass the Render Pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	// Need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpass_deps;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// Transition must happen after...
	subpass_deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
	subpass_deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;	// Pipeline stage
	subpass_deps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
	// But must happen before...
	subpass_deps[0].dstSubpass = 0;
	subpass_deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_deps[0].dependencyFlags = 0;


	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	subpass_deps[1].srcSubpass = 0;
	subpass_deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
	// But must happen before...
	subpass_deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpass_deps[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_deps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpass_deps[1].dependencyFlags = 0;

	// Create info for Render Pass
	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = static_cast<uint32_t>(subpass_deps.size());
	render_pass_info.pDependencies = subpass_deps.data();

	VKRes(vkCreateRenderPass(mainDevice.logical_device, &render_pass_info, nullptr, &render_pass));
}

void VulkanRenderer::createFramebuffers() {
	swapchain_framebuffers.resize(swapchain_images.size());
	for (size_t i = 0; i < swapchain_framebuffers.size(); i++) {
		std::array<VkImageView, 1> attachments = {
			swapchain_images[i].image_view
		};

		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass = render_pass;
		fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		fb_info.pAttachments = attachments.data();
		fb_info.width = sc_extent.width;
		fb_info.height = sc_extent.height;
		fb_info.layers = 1;

		VKRes(vkCreateFramebuffer(mainDevice.logical_device, &fb_info, nullptr, &swapchain_framebuffers[i]));
	}
}

void VulkanRenderer::createCommandPool() {
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = indices.graphics_family;

	VKRes(vkCreateCommandPool(mainDevice.logical_device, &pool_info, nullptr, &graphics_command_pool));
}

void VulkanRenderer::createCommandBuffers() {
	command_buffers.resize(swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo cb_alloc_info = {};
	cb_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cb_alloc_info.commandPool = graphics_command_pool;
	cb_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	cb_alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

	// Alloc cmd buffs and place handles in array of buffers
	VKRes(vkAllocateCommandBuffers(mainDevice.logical_device, &cb_alloc_info, command_buffers.data()));
}

void VulkanRenderer::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding vp_binding = {};
	vp_binding.binding = 0;
	vp_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vp_binding.descriptorCount = 1;
	vp_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vp_binding.pImmutableSamplers = nullptr;

	// VkDescriptorSetLayoutBinding model_binding = {};
	// model_binding.binding = 1;
	// model_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// model_binding.descriptorCount = 1;
	// model_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// model_binding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layout_bindings = { vp_binding };

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<u32>(layout_bindings.size());
	layout_info.pBindings = layout_bindings.data();

	VKRes(vkCreateDescriptorSetLayout(mainDevice.logical_device, &layout_info, nullptr, &descriptor_set_layout));



}

void VulkanRenderer::createUniformBuffers() {
	VkDeviceSize buffersize = sizeof(UboViewProjection);
	
	//VkDeviceSize model_buffersize = model_uniform_alignment * MAX_OBJECTS;



	vp_uniform_buffer.resize(swapchain_images.size());
	vp_uniform_buffer_memory.resize(swapchain_images.size());

	// model_uniform_buffer.resize(swapchain_images.size());
	// model_uniform_buffer_memory.resize(swapchain_images.size());

	for (size_t i = 0; i < swapchain_images.size(); i++) {
		createBuffer(mainDevice.physical_device, mainDevice.logical_device, buffersize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vp_uniform_buffer[i], &vp_uniform_buffer_memory[i]);
		// createBuffer(mainDevice.physical_device, mainDevice.logical_device, model_buffersize,
		// 	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		// 	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		// 	&model_uniform_buffer[i], &model_uniform_buffer_memory[i]);
	}
}

void VulkanRenderer::createDescriptorPool() {

	VkDescriptorPoolSize vp_poolsize = {};
	vp_poolsize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vp_poolsize.descriptorCount = static_cast<u32>(vp_uniform_buffer.size());
	
	// VkDescriptorPoolSize model_poolsize = {};
	// model_poolsize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// model_poolsize.descriptorCount = static_cast<u32>(model_uniform_buffer.size());
	
	std::vector<VkDescriptorPoolSize> pool_sizes = { vp_poolsize };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = static_cast<u32>(swapchain_images.size());
	pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());;
	pool_info.pPoolSizes = pool_sizes.data();

	VKRes(vkCreateDescriptorPool(mainDevice.logical_device, &pool_info, nullptr, &descriptor_pool));
}

void VulkanRenderer::createDescriptorSets() {
	descriptor_sets.resize(swapchain_images.size());

	std::vector<VkDescriptorSetLayout> setlayouts(swapchain_images.size(), descriptor_set_layout);

	VkDescriptorSetAllocateInfo set_alloc_info = {};
	set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_alloc_info.descriptorPool = descriptor_pool;
	set_alloc_info.descriptorSetCount = static_cast<u32>(swapchain_images.size());
	set_alloc_info.pSetLayouts = setlayouts.data();

	VKRes(vkAllocateDescriptorSets(mainDevice.logical_device, &set_alloc_info, descriptor_sets.data()));

	for (size_t i = 0; i < swapchain_images.size(); i++) {
		// View Proj
		VkDescriptorBufferInfo vp_buff_info = {};
		vp_buff_info.buffer = vp_uniform_buffer[i];
		vp_buff_info.offset = 0;
		vp_buff_info.range = sizeof(UboViewProjection);

		VkWriteDescriptorSet vp_write = {};
		vp_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vp_write.dstSet = descriptor_sets[i];
		vp_write.dstBinding = 0;
		vp_write.dstArrayElement = 0;
		vp_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vp_write.descriptorCount = 1;
		vp_write.pBufferInfo = &vp_buff_info;

		// Model descriptor
		// VkDescriptorBufferInfo model_buff_info = {};
		// model_buff_info.buffer = model_uniform_buffer[i];
		// model_buff_info.offset = 0;
		// model_buff_info.range = model_uniform_alignment;
		// 
		// VkWriteDescriptorSet model_write = {};
		// model_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// model_write.dstSet = descriptor_sets[i];
		// model_write.dstBinding = 1;
		// model_write.dstArrayElement = 0;
		// model_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		// model_write.descriptorCount = 1;
		// model_write.pBufferInfo = &model_buff_info;

		std::vector<VkWriteDescriptorSet> set_writes = { vp_write };

		vkUpdateDescriptorSets(mainDevice.logical_device, 
			static_cast<u32>(set_writes.size()), set_writes.data(), 0, nullptr);
	}
}

void VulkanRenderer::updateUniformBuffers(u32 img_idx) {
	void* data;
	vkMapMemory(mainDevice.logical_device, vp_uniform_buffer_memory[img_idx], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &ubo_view_proj, sizeof(UboViewProjection));
	vkUnmapMemory(mainDevice.logical_device, vp_uniform_buffer_memory[img_idx]);

	// for (size_t i = 0; i < meshes.size(); i++) {
	// 	Model* model = (Model*)((u64)model_transfer_space + (i * model_uniform_alignment));
	// 	*model = meshes[i].getModel();
	// }
	// vkMapMemory(mainDevice.logical_device, model_uniform_buffer_memory[img_idx],
	// 	0, model_uniform_alignment* meshes.size(), 0, &data);
	// memcpy(data, model_transfer_space, model_uniform_alignment * meshes.size());
	// vkUnmapMemory(mainDevice.logical_device, model_uniform_buffer_memory[img_idx]);
}

void VulkanRenderer::recordCommands(u32 curr_img) {
	VkCommandBufferBeginInfo buff_begin_info = {};
	buff_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//buff_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;		// use before synchronization

	// how to being a render pass (only for graphics)
	VkRenderPassBeginInfo rp_begin_info = {};
	rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin_info.renderPass = render_pass;
	rp_begin_info.renderArea.offset = { 0,0 };
	rp_begin_info.renderArea.extent = sc_extent;
	VkClearValue clear_values[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	rp_begin_info.pClearValues = clear_values;
	rp_begin_info.clearValueCount = 1;

	rp_begin_info.framebuffer = swapchain_framebuffers[curr_img];

	// Start recording commands to cmd buff
	VKRes(vkBeginCommandBuffer(command_buffers[curr_img], &buff_begin_info));

	vkCmdBeginRenderPass(command_buffers[curr_img], &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffers[curr_img], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	for (size_t j = 0; j < meshes.size(); j++) {
		VkBuffer vertex_buffers[] = { meshes[j].getVertexBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffers[curr_img], 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(command_buffers[curr_img], meshes[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		//u32 dynamic_offset = static_cast<u32>(model_uniform_alignment) * j;

		vkCmdPushConstants(command_buffers[curr_img], pipeline_layout,
			VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &meshes[j].getModel());

		vkCmdBindDescriptorSets(command_buffers[curr_img], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
			&descriptor_sets[curr_img], 0, nullptr);

		//vkCmdDraw(command_buffers[i], first_mesh.getVertexCount(), 1, 0, 0);
		vkCmdDrawIndexed(command_buffers[curr_img], meshes[j].getIndexCount(), 1, 0, 0, 0);
	}
	vkCmdEndRenderPass(command_buffers[curr_img]);

	// Stop recording to cmd buff
	VKRes(vkEndCommandBuffer(command_buffers[curr_img]));

}

void VulkanRenderer::createSynchronisation() {

	image_available.resize(MAX_FRAME_DRAWS);
	render_finished.resize(MAX_FRAME_DRAWS);
	draw_fences.resize(MAX_FRAME_DRAWS);

	VkSemaphoreCreateInfo sem_info = {};
	sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
		VKRes(vkCreateSemaphore(mainDevice.logical_device, &sem_info, nullptr, &image_available[i]));
		VKRes(vkCreateSemaphore(mainDevice.logical_device, &sem_info, nullptr, &render_finished[i]));
		VKRes(vkCreateFence(mainDevice.logical_device, &fence_info, nullptr, &draw_fences[i]));
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

	// vertex creation
	// data for a single vertex as a whole
	VkVertexInputBindingDescription binding_desc = {};
	binding_desc.binding = 0;
	binding_desc.stride = sizeof(Vertex);
	binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// how data for an attibute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 2> attrib_descs;
	attrib_descs[0].binding = 0;
	attrib_descs[0].location = 0;
	attrib_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_descs[0].offset = offsetof(Vertex, pos);

	// colors
	attrib_descs[1].binding = 0;
	attrib_descs[1].location = 1;
	attrib_descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_descs[1].offset = offsetof(Vertex, col);

	VkPipelineVertexInputStateCreateInfo vertex_in_info = {};
	vertex_in_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_in_info.vertexBindingDescriptionCount = 1;
	vertex_in_info.pVertexBindingDescriptions = &binding_desc;		//(binding desc (data spacing/stride, etc.)
	vertex_in_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib_descs.size());
	vertex_in_info.pVertexAttributeDescriptions = attrib_descs.data();		// data format, where to bind to/from


	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)sc_extent.width;
	viewport.height = (float)sc_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = sc_extent;

	VkPipelineViewportStateCreateInfo vp_create_info = {};
	vp_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp_create_info.viewportCount = 1;
	vp_create_info.pViewports = &viewport;
	vp_create_info.scissorCount = 1;
	vp_create_info.pScissors = &scissor;

	// DYNAMIC STATES
	// std::vector<VkDynamicState> enabled_dynamic_states;
	// enabled_dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	// enabled_dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);
	// 
	// VkPipelineDynamicStateCreateInfo dynamic_info = {};
	// dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	// dynamic_info.dynamicStateCount = static_cast<uint32_t>(enabled_dynamic_states.size());
	// dynamic_info.pDynamicStates = enabled_dynamic_states.data();
	
	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
	rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_info.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterizer_info.rasterizerDiscardEnable = VK_FALSE;		// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;		// How to handle filling points between vertices
	rasterizer_info.lineWidth = 1.0f;						// How thick lines should be when drawn
	rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
	rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
	rasterizer_info.depthBiasEnable = VK_FALSE;				// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)


	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisampling_info = {};
	multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_info.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
	multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment


	// -- BLENDING --
	// Blending decides how to blend a new colour being written to a fragment, with the old value

	// Blend Attachment State (how blending is handled)
	VkPipelineColorBlendAttachmentState color_state = {};
	color_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_state.blendEnable = VK_TRUE;													// Enable blending

	// Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
	color_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_state.colorBlendOp = VK_BLEND_OP_ADD;

	// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
	//			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

	color_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_state.alphaBlendOp = VK_BLEND_OP_ADD;
	// Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo blending_info = {};
	blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blending_info.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
	blending_info.attachmentCount = 1;
	blending_info.pAttachments = &color_state;


	// -- PIPELINE LAYOUT --
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	// Create Pipeline Layout
	VKRes(vkCreatePipelineLayout(mainDevice.logical_device, &pipeline_layout_info, nullptr, &pipeline_layout));



	// -- DEPTH STENCIL TESTING --
	// TODO: Set up depth stencil testing


	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;									// Number of shader stages
	pipeline_create_info.pStages = shader_stages;							// List of shader stages
	pipeline_create_info.pVertexInputState = &vertex_in_info;				// All the fixed function pipeline states
	pipeline_create_info.pInputAssemblyState = &input_assembly;
	pipeline_create_info.pViewportState = &vp_create_info;
	pipeline_create_info.pDynamicState = nullptr;
	pipeline_create_info.pRasterizationState = &rasterizer_info;
	pipeline_create_info.pMultisampleState = &multisampling_info;
	pipeline_create_info.pColorBlendState = &blending_info;
	pipeline_create_info.pDepthStencilState = nullptr;
	pipeline_create_info.layout = pipeline_layout;							// Pipeline Layout pipeline should use
	pipeline_create_info.renderPass = render_pass;							// Render pass description the pipeline is compatible with
	pipeline_create_info.subpass = 0;										// Subpass of render pass to use with pipeline

	// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
	pipeline_create_info.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

	// Create Graphics Pipeline
	VKRes(vkCreateGraphicsPipelines(mainDevice.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline));

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

void VulkanRenderer::allocateDynamicBufferTransferSpace() {
	// Round to nearest alignment
	// model_uniform_alignment = (sizeof(Model) + min_uniform_buff_offset-1) 
	// 							& ~(min_uniform_buff_offset - 1);
	// 
	// model_transfer_space = (Model*)_aligned_malloc(model_uniform_alignment * MAX_OBJECTS, model_uniform_alignment);

}

void VulkanRenderer::createPushConstantRange() {
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(Model);
}
