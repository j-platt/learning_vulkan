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

public:
	HelloTriangleApplication() : window(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr))
    {
        initWindow();
        initVulkan();
        mainLoop();
    }

	~HelloTriangleApplication() 
    {
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
	}

    void mainLoop() {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
	}

	void initWindow()
	{
		glfwInit();
		
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan Window", nullptr, nullptr);
	}
};

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