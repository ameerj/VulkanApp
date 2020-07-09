#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include "Mesh.h"
#include "Utilities.h"

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    int init(GLFWwindow* newWindow);

    void updateModel(int id, glm::mat4 new_model);

    void draw();
    void cleanup();

private:
    GLFWwindow* window;

    VkInstance instance;
    struct {
        VkPhysicalDevice physical_device;
        VkDevice logical_device;
    } mainDevice;
    VkQueue graphics_queue;
    VkQueue presentation_queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    std::vector<SwapchainImage> swapchain_images;
    std::vector<VkFramebuffer> swapchain_framebuffers;
    std::vector<VkCommandBuffer> command_buffers;

    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_image_memory;
    VkFormat depth_fmt;

    // - Pipeline
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;

    // - Pools
    VkCommandPool graphics_command_pool;

    // Validation
    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    // Utility
    VkFormat sc_img_format;
    VkExtent2D sc_extent;

    // VkDeviceSize min_uniform_buff_offset;
    // size_t model_uniform_alignment;

    int current_frame = 0;
    // Synchronisation
    std::vector<VkSemaphore> image_available;
    std::vector<VkSemaphore> render_finished;
    std::vector<VkFence> draw_fences;

    // --FUNCTIONS--

    // Vulkan functions
    void getPhysicalDevice();

    // - Create funcs
    void createInstance();
    void createLogicalDevice();
    void createDebugMessenger();
    void createSurface();
    void createSwapchain();
    void createGraphicsPipeline();
    void createRenderPass();
    void createDepthBufferImage();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();

    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void allocateDynamicBufferTransferSpace();

    void createPushConstantRange();

    void updateUniformBuffers(u32 img_idx);

    void recordCommands(u32 curr_img);
    void createSynchronisation();

    // - Support funcs
    bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtenstions);
    bool checkDeviceInstanceExtensionsSupport(VkPhysicalDevice device);
    bool checkDeviceSuitable(VkPhysicalDevice device);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseBestFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseBestPresMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling,
                                   VkFormatFeatureFlags flags);

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                              VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks* pAllocator);

    VkImage createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags flags, VkMemoryPropertyFlags propflags,
                        VkDeviceMemory* imagemem);

    VkImageView createIMageView(VkImage image, VkFormat format, VkImageAspectFlags flags);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Scene objects
    std::vector<Mesh> meshes;

    // Scene settings
    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } ubo_view_proj;

    // Descriptors
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    std::vector<VkBuffer> vp_uniform_buffer;
    std::vector<VkDeviceMemory> vp_uniform_buffer_memory;

    std::vector<VkBuffer> model_uniform_buffer;
    std::vector<VkDeviceMemory> model_uniform_buffer_memory;

    Model* model_transfer_space;

    VkPushConstantRange push_constant_range;
};
