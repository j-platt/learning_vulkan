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

#pragma warning(disable: 26812) //enum class warning
#pragma warning(disable: 26495) //unitialized member warning

#define VK_FAILED(tested) (tested != VK_SUCCESS)

using std::for_each;
using std::begin;
using std::end;
using std::vector;

using queue_family_index_t = uint32_t;
using queue_family_indices = struct queue_familyindex_struct
{
    std::optional<queue_family_index_t> graphicsFamily;
    std::optional<queue_family_index_t> presentationFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentationFamily.has_value(); }
};
using image_list = vector<VkImage>;
using image_views = vector<VkImageView>;

struct swap_chain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> presentModes;
};

constexpr uint32_t windowWidth = 800;
constexpr uint32_t windowHeight = 600;
constexpr VkQueueFlagBits queueRequirements = VK_QUEUE_GRAPHICS_BIT;

vector<char const*> const validationLayers =
{
    "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_LUNARG_api_dump",
};

vector<char const*> const requiredExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VkInstance createInstance();

void check_specified_validation_layers_supported();

VkPhysicalDevice pickPhysicalDevice(VkInstance const& vulkanInstance, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<char const*> const& requiredExtensions);

//todo: split into three functions
std::tuple<VkDevice, queue_family_index_t, queue_family_index_t> createLogicalDevice(VkPhysicalDevice const& physicalDevice, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<const char*> const& deviceExtensions);

VkSurfaceKHR createSurface(VkInstance const& instance, GLFWwindow* window);

swap_chain_support_details querySwapChainSupport(VkPhysicalDevice const& device, VkSurfaceKHR const& surface);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(vector<VkSurfaceFormatKHR> const& availableFormats);

VkPresentModeKHR chooseSwapPresentMode(vector<VkPresentModeKHR> const& availablePresentModes);

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR const& capabilities);

VkSwapchainKHR createSwapChain(swap_chain_support_details const& swapChainSupport, VkSurfaceKHR const& surface, VkPhysicalDevice const& physicalDevice, VkDevice const& logicalDevice);

image_views createImageViews(image_list const& images, VkFormat const& format, VkDevice const& logicalDevice);

//todo: see if there is a better way to destory pipeline layout
std::tuple<VkPipeline, VkPipelineLayout> createGraphicsPipeline(VkDevice const& logicalDevice, VkExtent2D const& swapchainExtent, VkRenderPass const& renderPass);

vector<char> readFile(std::string const& fileName);

VkShaderModule createShaderModule(vector<char> const& code, VkDevice const& logicalDevice);

VkRenderPass createRenderPass(VkDevice const& logicalDevice, VkFormat const& format);