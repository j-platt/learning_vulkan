#include "vulkan_init.h"
#include <fstream>

swap_chain_support_details querySwapChainSupport(VkPhysicalDevice const& device, VkSurfaceKHR const& surface)
{
    swap_chain_support_details reply;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &reply.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if(formatCount)
    {
        reply.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, reply.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if(presentModeCount)
    {
        reply.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, reply.presentModes.data());
    }

    return reply;
}

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
    for(char const* specifiedLayer : validationLayers)
    {
        bool layerFound = false;
        for(VkLayerProperties const& availableLayer : availableLayers)
        {
            if(std::strcmp(availableLayer.layerName, specifiedLayer) == 0)
            {
                layerFound = true;
            }
        }
        if(!layerFound)
        {
            unsupportedLayers.emplace_back(specifiedLayer);
        }
    }
    if constexpr(enableValidationLayers)
    {
        if(!unsupportedLayers.empty())
        {
            std::string errMsg = "\nThese validation layers were requested but not supported:\n";
            for (std::string const& unsupportedLayer : unsupportedLayers)
            {
                errMsg += '\t' + unsupportedLayer + '\n';
            }
            errMsg += '\n';
            throw std::runtime_error(errMsg);
        }
    }
}

queue_family_indices findQueueFamilies(VkPhysicalDevice const& device, VkQueueFlagBits const flags, VkSurfaceKHR const& surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = 0;
    queue_family_indices reply;
    for(auto const& queueFamily : queueFamilies)
    {
        if(queueFamily.queueFlags & flags)
        {
            reply.graphicsFamily = index;
        }
        VkBool32 supportsPresentation = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresentation);
        if(supportsPresentation)
        {
            reply.presentationFamily = index;
        }
        if(reply.isComplete())
        {
            break;
        }
        ++index;
    };
    return reply;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice const& toCheck, vector<char const*> requiredExtensions)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(toCheck, nullptr, &extensionCount, nullptr);
    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(toCheck, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtSet(begin(requiredExtensions), end(requiredExtensions));
    for(VkExtensionProperties const& extension : availableExtensions)
    {
        requiredExtSet.erase(extension.extensionName);
    }
    //it would be good to be able to report here which are missing
    return requiredExtSet.empty();
}

bool deviceIsSuitable(VkPhysicalDevice const& toCheck, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<char const*> requiredExtensions)
{
    //VkPhysicalDeviceProperties about;
    //vkGetPhysicalDeviceProperties(toCheck, &about);

    //VkPhysicalDeviceFeatures features;
    //vkGetPhysicalDeviceFeatures(toCheck, &features);

    bool const extensionsSupported = checkDeviceExtensionSupport(toCheck, requiredExtensions);
    bool swapChainAdequate = false;
    if(extensionsSupported)
    {
        swap_chain_support_details swapChainSupport = querySwapChainSupport(toCheck, surface);
        swapChainAdequate = !(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty());
    }
    return findQueueFamilies(toCheck, requirements, surface).isComplete() && extensionsSupported && swapChainAdequate;
}

VkPhysicalDevice pickPhysicalDevice(VkInstance const& vulkanInstance, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<char const*> const& requiredExtensions)
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
        if(deviceIsSuitable(device, surface, requirements, requiredExtensions))
        {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU.");
    }
    return physicalDevice;
}

std::tuple<VkDevice, queue_family_index_t, queue_family_index_t> createLogicalDevice(VkPhysicalDevice const& physicalDevice, VkSurfaceKHR const& surface, VkQueueFlagBits const requirements, vector<const char*> const& deviceExtensions)
{
    queue_family_indices indices = findQueueFamilies(physicalDevice, requirements, surface);
    if(!indices.isComplete())
    {
        throw std::runtime_error("Failed to find the required queue families.");
    }

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<queue_family_index_t> const uniqueQueueFamilies =
    {
        indices.graphicsFamily.value(),
        indices.presentationFamily.value(),
    };

    constexpr float queuePriority = 1.0f;
    VkPhysicalDeviceFeatures deviceFeatures{};

    for(queue_family_index_t const queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice logicalDevice;
    if(VK_FAILED(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice)))
    {
        throw std::runtime_error("Failed to create the logical device.");
    }
    return 
    {
        logicalDevice, 
        indices.graphicsFamily.value(), 
        indices.presentationFamily.value()
    };
}

VkSurfaceKHR createSurface(VkInstance const& instance, GLFWwindow* window)
{
    VkSurfaceKHR reply{};
    if(VK_FAILED(glfwCreateWindowSurface(instance, window, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create the window surface.");
    }
    return reply;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(vector<VkSurfaceFormatKHR> const& availableFormats)
{
    for(VkSurfaceFormatKHR const& format : availableFormats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    return availableFormats.front();
}

VkPresentModeKHR chooseSwapPresentMode(vector<VkPresentModeKHR> const& availablePresentModes)
{
    for(VkPresentModeKHR const& presentMode : availablePresentModes)
    {
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR const& capabilities)
{
    if(capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { windowWidth, windowHeight };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkSwapchainKHR createSwapChain(swap_chain_support_details const& swapChainSupport, VkSurfaceKHR const& surface, VkPhysicalDevice const& physicalDevice, VkDevice const& logicalDevice)
{
    VkSurfaceFormatKHR const surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR const presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D const extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR creationInfo{};
    creationInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    creationInfo.surface = surface;
    creationInfo.minImageCount = imageCount;
    creationInfo.imageFormat = surfaceFormat.format;
    creationInfo.imageColorSpace = surfaceFormat.colorSpace;
    creationInfo.imageExtent = extent;
    creationInfo.imageArrayLayers = 1;
    creationInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queue_family_indices indices = findQueueFamilies(physicalDevice, queueRequirements, surface);
    uint32_t const queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };
    if(indices.graphicsFamily != indices.presentationFamily)
    {
        creationInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        creationInfo.queueFamilyIndexCount = 2;
        creationInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        creationInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    creationInfo.preTransform = swapChainSupport.capabilities.currentTransform;//dont apply transform to images
    creationInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//don't bother being a transparent layer in windows
    creationInfo.presentMode = presentMode;
    creationInfo.clipped = VK_TRUE;//don't care about obscured pixels, this would be a bad choice for system testing but is more performant.
    creationInfo.oldSwapchain = VK_NULL_HANDLE;//for when previous swapchain is invalidated e.g. by resizing window, ignored for first project to keep simple.

    VkSwapchainKHR reply;
    if(VK_FAILED(vkCreateSwapchainKHR(logicalDevice, &creationInfo, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create the swap chain.");
    }
    return reply;
}

image_views createImageViews(image_list const& images, VkFormat const& format, VkDevice const& logicalDevice)
{
    vector<VkImageView> reply;
    reply.resize(images.size());
    for(VkImage const& image : images)
    {
        VkImageViewCreateInfo creationInfo{};
        creationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        creationInfo.image = image;
        creationInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        creationInfo.format = format;
        creationInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        creationInfo.subresourceRange.baseMipLevel = 0;
        creationInfo.subresourceRange.levelCount = 1;
        creationInfo.subresourceRange.baseArrayLayer = 0;
        creationInfo.subresourceRange.layerCount = 1;
        if(VK_FAILED(vkCreateImageView(logicalDevice, &creationInfo, nullptr, &reply[&image - &images[0]])))
        {
            throw std::runtime_error("Failed to create the image views.");
        }
    }
    return reply;
}

std::tuple<VkPipeline, VkPipelineLayout> createGraphicsPipeline(VkDevice const& logicalDevice, VkExtent2D const& swapchainExtent, VkRenderPass const& renderPass)
{
    //todo: combine first 2 steps if possible
    vector<char> vertShaderCode = readFile("shaders/vert.spv");
    vector<char> fragShaderCode = readFile("shaders/frag.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, logicalDevice);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, logicalDevice);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] =
    {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout pipelineLayout;
    if(VK_FAILED(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout)))
    {
        throw std::runtime_error("Failed to create the pipeline layout.");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    VkPipeline reply;
    if(VK_FAILED(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create graphics pipeline.");
    }

    //todo: these wont be called if error is thrown above, needs raii
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);

    return { reply, pipelineLayout };
}

vector<char> readFile(std::string const& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if(!file.is_open())
    {
        throw std::runtime_error("Failed to open " + fileName);
    }
    size_t fileSize = file.tellg();
    vector<char> reply(fileSize);
    file.seekg(0);
    file.read(reply.data(), fileSize);
    file.close();
    return reply;
}

VkShaderModule createShaderModule(vector<char> const& code, VkDevice const& logicalDevice)
{
    VkShaderModuleCreateInfo creationInfo{};
    creationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    creationInfo.codeSize = code.size();
    creationInfo.pCode = reinterpret_cast<uint32_t const*>(code.data());
    VkShaderModule reply;
    if(VK_FAILED(vkCreateShaderModule(logicalDevice, &creationInfo, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create a shader module.");
    }
    return reply;
}

VkRenderPass createRenderPass(VkDevice const& logicalDevice, VkFormat const& format)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass reply;
    if(VK_FAILED(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create the render pass.");
    }
    return reply;
}

vector<VkFramebuffer> createFreamebuffers(VkDevice const& logicalDevice, image_views const& imageViews, VkRenderPass const& renderPass, VkExtent2D const& extent)
{
    vector<VkFramebuffer> reply;
    reply.resize(imageViews.size());

    for(VkImageView const& imageView : imageViews)
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imageView;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if(VK_FAILED(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &reply[&imageView - &imageViews[0]])))
        {
            throw std::runtime_error("Failed to create framebuffer.");
        }
    }
    return reply;
}

VkCommandPool createCommandPool(VkDevice const& logicalDevice, queue_family_index_t const& graphicsFamily)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsFamily;

    VkCommandPool reply;
    if(VK_FAILED(vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &reply)))
    {
        throw std::runtime_error("Failed to create command pool.");
    }
    return reply;
}

vector<VkCommandBuffer> createCommandBuffers(VkDevice const& logicalDevice,
    VkCommandPool const& commandPool,
    uint32_t const& frameBufferCount,
    VkRenderPass const& renderPass,
    vector<VkFramebuffer> const& frameBuffers,
    VkExtent2D const& extent,
    VkPipeline const& graphicsPipeline)
{
    vector<VkCommandBuffer> reply;
    reply.resize(frameBufferCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(reply.size());

    if(VK_FAILED(vkAllocateCommandBuffers(logicalDevice, &allocInfo, reply.data())))
    {
        throw std::runtime_error("Failed to allocate command buffers.");
    }

    for(VkCommandBuffer const& commandBuffer : reply)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if(VK_FAILED(vkBeginCommandBuffer(commandBuffer, &beginInfo)))
        {
            throw std::runtime_error("Failed to begin recording command buffer.");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffers[&commandBuffer - &reply[0]];
        renderPassInfo.renderArea.offset = { 0,0 };
        renderPassInfo.renderArea.extent = extent;
        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        if(VK_FAILED(vkEndCommandBuffer(commandBuffer)))
        {
            throw std::runtime_error("Failed to record command buffer.");
        }
    }
    return reply;
}