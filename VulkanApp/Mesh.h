#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vector>
#include <GLFW/glfw3.h>
#include "Utilities.h"

struct Model {
    glm::mat4 model;
};

class Mesh {
public:
    Mesh();
    Mesh(VkPhysicalDevice physical_device, VkDevice device, VkQueue transfer_queue,
         VkCommandPool transfer_cmd_pool, std::vector<Vertex>* vertices, std::vector<u32>* indices,
         int new_texid);

    int getVertexCount();
    VkBuffer getVertexBuffer();

    int getIndexCount();
    VkBuffer getIndexBuffer();
    void destroyBuffers();

    int getTexId();

    void setModel(glm::mat4 new_model);
    Model getModel() {
        return model;
    }

    ~Mesh();

private:
    Model model;
    int vertex_count;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    int index_count;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    int tex_id;

    VkPhysicalDevice physical_device;
    VkDevice device;

    void createVertexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
                            std::vector<Vertex>* vertices);
    void createIndexBuffer(VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
                           std::vector<u32>* indices);
};
