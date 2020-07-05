#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vk_renderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600) {
	// initialize glfw
	glfwInit();

	// glfw to not work with opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() {

	//create window
	initWindow();
	if (vk_renderer.init(window) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float delta_time = 0.0f;
	float last_time = 0.0f;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float now = glfwGetTime();
		delta_time = now - last_time;
		last_time = now;

		angle += 10.0f * delta_time;
		if (angle > 360.0f) angle -= 360.0f;

		vk_renderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f)));

		vk_renderer.draw();
	}

	vk_renderer.cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
