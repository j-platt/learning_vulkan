#include "vulkan_init.h"

class HelloTriangleApplication {
	GLFWwindow* window;
    VkInstance vulkanInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    image_list swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    image_views swapChainImageViews;
    VkPipeline graphicsPipeline;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    vector<VkCommandBuffer> commandBuffers;

    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores;
    vector<VkFence> inFlightFences;
    vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

public:
	HelloTriangleApplication() : window(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr))
    {
        initWindow();
        initVulkan();
        mainLoop();
    }

	~HelloTriangleApplication()
    {
        for(VkSemaphore const& semaphore : renderFinishedSemaphores)
        {
            vkDestroySemaphore(logicalDevice, semaphore, nullptr);
        }
        for(VkSemaphore const& semaphore : imageAvailableSemaphores)
        {
            vkDestroySemaphore(logicalDevice, semaphore, nullptr);
        }
        for(VkFence const& fence : inFlightFences)
        {
            vkDestroyFence(logicalDevice, fence, nullptr);
        }
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        for(VkFramebuffer const& framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        for(VkImageView const& imageView : swapChainImageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
        vkDestroyInstance(vulkanInstance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
	}

private:
	void initVulkan() {
        vulkanInstance = createInstance();
        surface = createSurface(vulkanInstance, window);
        physicalDevice = pickPhysicalDevice(vulkanInstance, surface, queueRequirements, requiredExtensions);
        
        auto const[logicalDeviceResult, graphicsQueueIndex, presentationQueueIndex] = createLogicalDevice(physicalDevice, surface, queueRequirements, requiredExtensions);
        logicalDevice = logicalDeviceResult;
        vkGetDeviceQueue(logicalDevice, graphicsQueueIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, presentationQueueIndex, 0, &presentationQueue);
        
        swap_chain_support_details swapChainSupport = querySwapChainSupport(physicalDevice, surface);
        swapChain = createSwapChain(swapChainSupport, surface, physicalDevice, logicalDevice);
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat = chooseSwapSurfaceFormat(swapChainSupport.formats).format;
        swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);
        swapChainImageViews = createImageViews(swapChainImages, swapChainImageFormat, logicalDevice);

        renderPass = createRenderPass(logicalDevice, swapChainImageFormat);
        auto const[graphicsPipelineResult, pipelineLayoutResult] = createGraphicsPipeline(logicalDevice, swapChainExtent, renderPass);
        graphicsPipeline = graphicsPipelineResult;
        pipelineLayout = pipelineLayoutResult;

        swapChainFramebuffers = createFreamebuffers(logicalDevice, swapChainImageViews, renderPass, swapChainExtent);

        commandPool = createCommandPool(logicalDevice, graphicsQueueIndex);
        commandBuffers = createCommandBuffers(logicalDevice, commandPool, swapChainFramebuffers.size(), renderPass, swapChainFramebuffers, swapChainExtent, graphicsPipeline);
        createSemaphores();
	}

    void mainLoop() {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(logicalDevice);
	}

	void initWindow()
	{
		glfwInit();
		
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan Window", nullptr, nullptr);
	}


    void createSemaphores()
    {
        imageAvailableSemaphores.resize(maxFramesInFlight);
        renderFinishedSemaphores.resize(maxFramesInFlight);
        inFlightFences.resize(maxFramesInFlight);
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < maxFramesInFlight; ++i)
        {
            if(VK_FAILED(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]))
                || VK_FAILED(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]))
                || VK_FAILED(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i])))
            {
                throw std::runtime_error("Failed to create semaphores.");
            }
        }
    }

    void drawFrame()
    {
        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if(imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
        if(VK_FAILED(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame])))
        {
            throw std::runtime_error("Failed to submit draw command buffer.");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentationQueue, &presentInfo);

        currentFrame = (++currentFrame) % maxFramesInFlight;
    }
};

//TODO: make shader compilation a build step.
int main() {
	try 
    {
        HelloTriangleApplication app;
    }
	catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}