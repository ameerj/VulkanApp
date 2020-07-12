#pragma once

#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static bool VKCheckError(VkResult res, const char* file, int line) {
    if (res != VK_SUCCESS) {
        std::cout << "[Vulkan Error] (" << res << "): " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

#define ASSERT(x)                                                                                  \
    if (!(x))                                                                                      \
        __debugbreak();
#define VKRes(x) ASSERT(VKCheckError(x, __FILE__, __LINE__))

const std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 500;

struct Vertex {
    glm::vec3 pos; // position xyz
    glm::vec3 col; // color rgb
    glm::vec2 tex; // texture coords uv
};

typedef struct VkDev {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
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

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physcial_device, uint32_t allowed_types,
                                    VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memprops;
    vkGetPhysicalDeviceMemoryProperties(physcial_device, &memprops);

    for (u32 i = 0; i < memprops.memoryTypeCount; i++) {
        if ((allowed_types & (1 << i)) &&
            (memprops.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
}

static void createBuffer(VkPhysicalDevice physcial_device, VkDevice device,
                         VkDeviceSize buffer_size, VkBufferUsageFlags buffer_usage,
                         VkMemoryPropertyFlags properties, VkBuffer* buffer,
                         VkDeviceMemory* buffermem) {
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
    memalloc_info.memoryTypeIndex =
        findMemoryTypeIndex(physcial_device, memreqs.memoryTypeBits, properties);

    VKRes(vkAllocateMemory(device, &memalloc_info, nullptr, buffermem));

    vkBindBufferMemory(device, *buffer, *buffermem, 0);
}

static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool command_pool) {
    VkCommandBuffer cmd_buffer;

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_buffer, &begin_info);
    return cmd_buffer;
}

static void endCommandBuffer(VkDevice device, VkCommandPool command_pool, VkQueue queue,
                             VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

static void copyBuffer(VkDevice device, VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
                       VkBuffer src, VkBuffer dst, VkDeviceSize buffer_size) {

    VkCommandBuffer transfer_cmd_buffer = beginCommandBuffer(device, transfer_cmd_pool);

    VkBufferCopy buffer_copy_region = {};
    buffer_copy_region.srcOffset = 0;
    buffer_copy_region.dstOffset = 0;
    buffer_copy_region.size = buffer_size;

    vkCmdCopyBuffer(transfer_cmd_buffer, src, dst, 1, &buffer_copy_region);

    endCommandBuffer(device, transfer_cmd_pool, transfer_queue, transfer_cmd_buffer);
}

static void copyImageBuffer(VkDevice device, VkQueue transfer_queue,
                            VkCommandPool transfer_cmd_pool, VkBuffer src, VkImage dst, u32 width,
                            u32 height) {

    VkCommandBuffer transfer_cmd_buffer = beginCommandBuffer(device, transfer_cmd_pool);

    VkBufferImageCopy img_region = {};
    img_region.bufferOffset = 0;
    img_region.bufferRowLength = 0;
    img_region.bufferImageHeight = 0;
    img_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_region.imageSubresource.mipLevel = 0;
    img_region.imageSubresource.baseArrayLayer = 0;
    img_region.imageSubresource.layerCount = 1;
    img_region.imageOffset = {0, 0, 0};
    img_region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(transfer_cmd_buffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &img_region);

    endCommandBuffer(device, transfer_cmd_pool, transfer_queue, transfer_cmd_buffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool command_pool,
                                  VkImage image, VkImageLayout old_layout,
                                  VkImageLayout new_layout) {
    VkCommandBuffer cmd_buffer = beginCommandBuffer(device, command_pool);

    VkImageMemoryBarrier mem_barrier = {};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.oldLayout = old_layout;
    mem_barrier.newLayout = new_layout;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = image;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        mem_barrier.srcAccessMask = 0;
        mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1,
                         &mem_barrier);

    endCommandBuffer(device, command_pool, queue, cmd_buffer);
}
