#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include "Primitives.h"


void VK_CHECK_RESULT(VkResult ret, std::string msg);
extern std::vector<Texture> textures_loaded;


namespace utils {	

	std::vector<Mesh> loadOBJ(std::string path, Vulkan_Backend& in_backend);

	std::vector<char> readFile(const std::string& filename);

	VkShaderModule createShaderModule(const std::vector<char>& code, const VkDevice& device);

}
