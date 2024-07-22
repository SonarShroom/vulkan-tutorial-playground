#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <limits>
#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

constexpr bool ENABLE_VK_VALIDATION_LAYERS
{
#ifdef NDEBUG
    false
#else
    true
#endif
};

constexpr std::array<const char*, 1> validationLayers   
{
#ifndef NDEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
};

constexpr std::array<const char*, 1> deviceExtensions
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT)
    {
        vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication {
public:
    HelloTriangleApplication()
    {
        initWindow();
        initVulkan();
    }

    ~HelloTriangleApplication()
    {
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void run()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)    
    {
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            {
                std::cout << "Validation layer info: " << pCallbackData->pMessage << "\n";
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            {
                std::cerr << "Validation layer warning: " << pCallbackData->pMessage << "\n";
            } break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            {
                std::cerr << "Validation layer error: " << pCallbackData->pMessage << "\n";
            } break;
            default:
            {
                std::cerr << "Validation layer info (unknown severity): " << pCallbackData->pMessage << "\n";
            } break;
        }
        return VK_FALSE;
    }

private:

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            setupDebugMessenger();
        }
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = HelloTriangleApplication::debugCallback;
        createInfo.pUserData = nullptr;
    }

    void setupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo {};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to setup debug messenger!");
        }
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount {};
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const auto& layer : validationLayers)
        {
            bool layerFound { };
            for (const auto& availableLayer : availableLayers)
            {
                if (std::strcmp(layer, availableLayer.layerName) == 0)
                {
                    std::cout << "Validation layer " << layer << " found!\n";
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound)
            {
                std::cout << "Required validation layer " << layer << " not found!\n";
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t numExts {};
        const char** exts = glfwGetRequiredInstanceExtensions(&numExts);

        std::vector<const char*> requiredExtensions(exts, exts + numExts);
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        uint32_t extCount {};
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

        std::cout << "Available extensions:\n";
        for (const auto& ext : extensions)
        {
            std::cout << '\t' << ext.extensionName << "\n";
        }

        for (const auto reqExt : requiredExtensions)
        {
            auto foundIt = std::find_if(extensions.begin(), extensions.end(), [reqExt](const auto& ext) {
                return std::strcmp(ext.extensionName, reqExt) == 0;
            });
            if (foundIt == extensions.end())
            {
                std::cout << "\t\tExtension " << reqExt << " required but not available!\n";
            }
            else
            {
                std::cout << "Extension " << reqExt << " found.\n";
            }
        }

        return requiredExtensions;
    }

    void createInstance()
    {
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            if (!checkValidationLayerSupport())
            {
                throw std::runtime_error("Validation layers requested, but not available.");
            }
        }
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
        };

        VkDebugUtilsMessengerCreateInfoEXT createInfo { };
        populateDebugMessengerCreateInfo(createInfo);

        VkInstanceCreateInfo instanceInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
        };
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            instanceInfo.enabledLayerCount = validationLayers.size();
            instanceInfo.ppEnabledLayerNames = validationLayers.data();
            instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &createInfo;
        }
        else
        {
            instanceInfo.enabledLayerCount = 0;
            instanceInfo.pNext = nullptr;
        }

        auto reqExts = getRequiredExtensions();
        instanceInfo.enabledExtensionCount = reqExts.size();
        instanceInfo.ppEnabledExtensionNames = reqExts.data();

        if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan instance!");
        }
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    struct QueueFamilyIndicies
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        auto isComplete() -> bool
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndicies findQueueFamilies(const VkPhysicalDevice& device)
    {
        QueueFamilyIndicies indicies;
        uint32_t queueFamilyCount { };
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indicies.graphicsFamily = i;
            }

            VkBool32 presentSupport { };
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indicies.presentFamily = i;
            }

            if (indicies.isComplete())
            {
                break;
            }
            i++;
        }
        return indicies;
    };
    
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& device)
    {
        uint32_t extensionCount {};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExts { deviceExtensions.begin(), deviceExtensions.end() };

        for (const auto& extension : availableExtensions) {
            requiredExts.erase(extension.extensionName);
        }

        return requiredExts.empty();
    }
    
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities { };
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t _formatCount { };
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &_formatCount, nullptr);
        if (_formatCount)
        {
            details.formats.resize(_formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &_formatCount, details.formats.data());
        }

        uint32_t _presentModeCount { };
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &_presentModeCount, nullptr);
        if (_presentModeCount)
        {
            details.presentModes.resize(_presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &_presentModeCount, details.presentModes.data());
        }

        return details;
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount { };
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (!deviceCount)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        const auto _isDeviceSuitable = [&](const VkPhysicalDevice& device)
        {
            /*VkPhysicalDeviceProperties props  { };
            VkPhysicalDeviceFeatures feats      { };
            vkGetPhysicalDeviceProperties(device, &props);
            vkGetPhysicalDeviceFeatures(device, &feats);*/

            bool _extensionsSupported = checkDeviceExtensionSupport(device);
            bool _swapChainAdequate { };
            if (_extensionsSupported)
            {
                auto _swapChainSupport { querySwapChainSupport(device) };
                _swapChainAdequate = !_swapChainSupport.formats.empty() && !_swapChainSupport.presentModes.empty();
            }

            return _extensionsSupported && _swapChainAdequate && findQueueFamilies(device).isComplete();
        };
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& _device : devices)
        {
            if (_isDeviceSuitable(_device))
            {
                physicalDevice = _device;
                break;
            }
        }
        if (!physicalDevice)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
        auto indicies = findQueueFamilies(physicalDevice);

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies { indicies.graphicsFamily.value(), indicies.presentFamily.value() };
        VkPhysicalDeviceFeatures deviceFeatures {};
        for (const auto& queueFamilyIndex : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkDeviceCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
        };
        if constexpr (ENABLE_VK_VALIDATION_LAYERS)
        {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, indicies.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indicies.presentFamily.value(), 0, &presentQueue);
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& _format : availableFormats)
        {
            if (_format.format == VK_FORMAT_B8G8R8A8_SRGB && _format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return _format;
            }
        }
        return availableFormats.at(0);
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& _mode : availablePresentModes)
        {
            if (_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return _mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width{ }, height{ };
            glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actualExtent{
                .width = std::clamp(static_cast<uint32_t>(width),
                    capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width),
                .height = std::clamp(static_cast<uint32_t>(height),
                    capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height)
            };
            return actualExtent;
        }
    }

    void createSwapChain()
    {
        const auto _swapChainSupport   { querySwapChainSupport(physicalDevice) };
        const auto& _capabilities      { _swapChainSupport.capabilities };

        const auto _surfaceFormat      { chooseSwapSurfaceFormat(_swapChainSupport.formats) };
        const auto _presentMode        { chooseSwapPresentMode(_swapChainSupport.presentModes) };
        const auto _extent             { chooseSwapExtent(_capabilities) };

        auto _imageCount               { _capabilities.minImageCount + 1 };
        if (_capabilities.maxImageCount > 0 && _imageCount > _capabilities.maxImageCount)
        {
            _imageCount = _capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = _imageCount,
            .imageFormat = _surfaceFormat.format,
            .imageColorSpace = _surfaceFormat.colorSpace,
            .imageExtent = _extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = _capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = _presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
        };

        auto indicies = findQueueFamilies(physicalDevice);
        std::array<uint32_t, 2> queueFamilyIndicies {
            indicies.graphicsFamily.value(), indicies.presentFamily.value()
        };
        if (indicies.graphicsFamily != indicies.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndicies.data();
        }
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain");
        }
        vkGetSwapchainImagesKHR(device, swapChain, &_imageCount, nullptr);
        swapChainImages.resize(_imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &_imageCount, swapChainImages.data());
        swapChainFormat = _surfaceFormat.format;
        swapChainExtent = _extent;
    }

    GLFWwindow* window                      { };

    VkInstance instance                     { };
    VkSurfaceKHR surface                    { };
    VkPhysicalDevice physicalDevice         { VK_NULL_HANDLE };
    VkDevice device                         { };
    VkQueue graphicsQueue                   { };
    VkQueue presentQueue                    { };
    VkSwapchainKHR swapChain                { };
    VkFormat swapChainFormat                { };
    VkExtent2D swapChainExtent              { };
    std::vector<VkImage> swapChainImages;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debugMessenger { };
#endif
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}