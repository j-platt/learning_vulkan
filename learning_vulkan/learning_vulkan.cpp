#include "vulkan_init.h"

class HelloTriangleApplication {
	GLFWwindow* window;
    VkInstance vulkanInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;

    static constexpr uint32_t windowWidth = 800;
    static constexpr uint32_t windowHeight = 600;
    static constexpr VkQueueFlagBits queueRequirements = VK_QUEUE_GRAPHICS_BIT;

public:
	HelloTriangleApplication() : window(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr)) {}

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
	}

	~HelloTriangleApplication() 
    {
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroyInstance(vulkanInstance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
	}

private:
	void initVulkan() {
        vulkanInstance = createInstance();
        physicalDevice = pickPhysicalDevice(vulkanInstance, queueRequirements);
        auto const[logicalDeviceResult, queueIndices] = createLogicalDevice(physicalDevice, queueRequirements);
        logicalDevice = logicalDeviceResult;
        vkGetDeviceQueue(logicalDevice, queueIndices, 0, &graphicsQueue);
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
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}