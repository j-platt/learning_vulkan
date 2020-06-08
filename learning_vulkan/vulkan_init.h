#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>
#include <set>

#define VK_FAILED(tested) (tested != VK_SUCCESS)

using std::for_each;
using std::begin;
using std::end;
using std::vector;

using queue_family_index_t = uint32_t;
using queue_family_indices = struct 
{
    std::optional<queue_family_index_t> graphicsFamily;
    std::optional<queue_family_index_t> presentationFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentationFamily.has_value(); }
};

vector<char const*> const validationLayers =
{
    "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_LUNARG_api_dump",
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VkInstance createInstance();

void check_specified_validation_layers_supported();

VkPhysicalDevice pickPhysicalDevice(VkInstance const& vulkanInstance, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<char const*> const& requiredExtensions);

std::tuple<VkDevice, queue_family_index_t, queue_family_index_t> createLogicalDevice(VkPhysicalDevice const& physicalDevice, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<const char*> const& deviceExtensions);

VkSurfaceKHR createSurface(VkInstance const& instance, GLFWwindow* window);
