#pragma once

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(expr)                                                                         \
    do {                                                                                       \
        VkResult result = expr;                                                                \
        if (result != VK_SUCCESS) {                                                            \
            Logger::critical("Vulkan error ({}): {}", result, string_VkResult(result));        \
            throw std::runtime_error(std::string("Vulkan error: ") + string_VkResult(result)); \
        }                                                                                      \
    } while (false)