#pragma once 
#include <vk_types.h>

namespace vkutil {

    bool load_shader_module(char const* filePath, VkDevice device, VkShaderModule* outShaderModule);

};