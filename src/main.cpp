#include <array>
#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <cstdlib>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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

constexpr std::array<const char*, 1> validationLayers {
#ifndef NDEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
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
        instanceInfo.enabledLayerCount = 0;

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

            return findQueueFamilies(device).isComplete();
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
            .enabledExtensionCount = 0,
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

    GLFWwindow* window                      { };

    VkInstance instance                     { };
    VkSurfaceKHR surface                    { };
    VkPhysicalDevice physicalDevice         { VK_NULL_HANDLE };
    VkDevice device                         { };
    VkQueue graphicsQueue                   { };
    VkQueue presentQueue                    { };

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