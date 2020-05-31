#include "vulkan_init.h"

VkInstance createInstance()
{
    if(enableValidationLayers)
    {
        check_specified_validation_layers_supported();
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo creationInfo{};
    creationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    creationInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    char const** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    creationInfo.enabledExtensionCount = glfwExtensionCount;
    creationInfo.ppEnabledExtensionNames = glfwExtensions;

    if(enableValidationLayers)
    {
        creationInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        creationInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        creationInfo.enabledLayerCount = 0;
    }

    VkInstance vulkanInstance;
    if(VK_FAILED(vkCreateInstance(&creationInfo, nullptr, &vulkanInstance)))
    {
        throw std::runtime_error("Failed to create vulkan instance.");
    }

    //uint32_t extensionCount = 0;
    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    //std::vector<VkExtensionProperties> extensions(extensionCount);
    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    //std::cout << "Available extensions:\n";
    //for_each(begin(extensions), end(extensions), [](auto const& extension) 
    //{
    //    std::cout << '\t' << extension.extensionName
    //    << '\n';
    //});
    return vulkanInstance;
}

void check_specified_validation_layers_supported()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    vector<std::string> unsupportedLayers;
    for_each(begin(validationLayers), end(validationLayers), [&](auto const& specifiedLayer)
    {
        bool layerFound = false;
        for_each(begin(availableLayers), end(availableLayers), [&](auto const& availableLayer)
        {
            if(std::strcmp(availableLayer.layerName, specifiedLayer) == 0)
            {
                layerFound = true;
            }
        });
        if(!layerFound)
        {
            unsupportedLayers.emplace_back(specifiedLayer);
        }
    });
    if(enableValidationLayers && !unsupportedLayers.empty())
    {
        std::string errMsg = "\nThese validation layers were requested but not supported:\n";
        for_each(begin(unsupportedLayers), end(unsupportedLayers), [&errMsg](auto const& unsupportedLayer)
        {
            errMsg += '\t' + unsupportedLayer + '\n';
        });
        errMsg += '\n';
        throw std::runtime_error(errMsg);
    }
}

queue_family_indices findQueueFamilies(VkPhysicalDevice const& device, VkQueueFlagBits const flags)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = 0;
    queue_family_indices reply;
    for_each(begin(queueFamilies), end(queueFamilies), [&](auto const& queueFamily)
    {
        if(queueFamily.queueFlags & flags)
        {
            reply = index;
        }
        ++index;
    });
    return reply;
}

bool deviceIsSuitable(VkPhysicalDevice const& toCheck, VkQueueFlagBits const requirements)
{
    //VkPhysicalDeviceProperties about;
    //vkGetPhysicalDeviceProperties(toCheck, &about);

    //VkPhysicalDeviceFeatures features;
    //vkGetPhysicalDeviceFeatures(toCheck, &features);
    return findQueueFamilies(toCheck, requirements).has_value();
}

VkPhysicalDevice pickPhysicalDevice(VkInstance const& vulkanInstance, VkQueueFlagBits const requirements)
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);
    if(deviceCount == 0)
    {
        throw std::runtime_error("Failed to find any GPUs with Vulkan support.");
    }
    vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

    for(VkPhysicalDevice const& device : devices)
    {
        if(deviceIsSuitable(device, requirements))
        {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to a suitable GPU.");
    }
    return physicalDevice;
}

std::pair<VkDevice, queue_family_index_t> createLogicalDevice(VkPhysicalDevice const& physicalDevice, VkQueueFlagBits const requirements)
{
    queue_family_indices indices = findQueueFamilies(physicalDevice, requirements);
    if(!indices.has_value())
    {
        throw std::runtime_error("Failed to find the required queue families.");
    }

    float queuePriority = 1.0f;
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkDevice logicalDevice;
    if(VK_FAILED(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice)))
    {
        throw std::runtime_error("Failed to create the logical device.");
    }
    return {logicalDevice, indices.value()};
}