#include "Application.h"
#include "utils.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>


#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

Application::Application(const AppCreateInfo& info)
    : createInfo(info),
    vkInstance(VK_NULL_HANDLE),
    vkDevice(VK_NULL_HANDLE),
    vkAllocator(VK_NULL_HANDLE),
    vkSurface(VK_NULL_HANDLE)
{
}

void Application::Run()
{
    InitSDL();
    InitVulkan();
    MainLoop();
    CleanUP();
}

void Application::InitSDL()
{
    chk(SDL_Init(SDL_INIT_VIDEO));
	chk(SDL_Vulkan_LoadLibrary(NULL));
}


void Application::InitVulkan()
{
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
    #else
    constexpr bool enableValidationLayers = true;
    #endif

    if constexpr (enableValidationLayers)
    {
        uint32_t layerCount { 0 };
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const VkLayerProperties& layerProperties : availableLayers)
            {
                if (std::strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                std::cerr << "Validation layer requested, but not available: " << layerName << '\n';
            }
        }
    }


    uint32_t instanceExtensionsCount { 0 };
    const char * const* instanceExtensions { SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };
    
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = createInfo.appName,
        .apiVersion = VK_API_VERSION_1_3
    };
    
    uint32_t layerCount { 0 };
    const char* const * layerNames = nullptr;

    if constexpr (enableValidationLayers)
    {
        layerCount = static_cast<uint32_t>(validationLayers.size());
        layerNames = validationLayers.data();
    }

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layerNames,
        .enabledExtensionCount = instanceExtensionsCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    chk(vkCreateInstance(&instanceCI, nullptr, &vkInstance));


    // Device 
    uint32_t deviceCount{0};
    chk(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> devices(deviceCount);
    chk(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data()));

    uint32_t deviceIndex{0};
    if (createInfo.argc > 1)
    {
        deviceIndex = std::atoi(createInfo.argv[1]);
        assert(deviceIndex < deviceCount);
    }

    VkPhysicalDeviceProperties2 deviceProperties {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
    
    Print(std::format("Selected Device: {}", deviceProperties.properties.deviceName));

    // Queue Family
    uint32_t queueFamilyCount {0};
    vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, queueFamilies.data() );

    uint32_t queueFamilyIndex {0};
    for (size_t i = 0; i < queueFamilies.size(); i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndex = i;
            break;
        }
    }
    chk(SDL_Vulkan_GetPresentationSupport(vkInstance, devices[deviceIndex], queueFamilyIndex));


    const float queueFamilyProperies { 1.0f };
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queueFamilyProperies
    };


    // Logical Device
    const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceVulkan12Features enabledVk12Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };
    VkPhysicalDeviceVulkan13Features enabledVk13Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    VkPhysicalDeviceFeatures enabledVk10Features {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledVk13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    chk(vkCreateDevice(devices[deviceIndex], &deviceCreateInfo, nullptr, &vkDevice));
    vkGetDeviceQueue(vkDevice, queueFamilyIndex, 0, &vkQueue);

    // VMA
    VmaVulkanFunctions vkFunctions {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };

    VmaAllocatorCreateInfo allocatorCreateInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = devices[deviceIndex],
        .device = vkDevice,
        .pVulkanFunctions = &vkFunctions,
        .instance = vkInstance
    };

    chk(vmaCreateAllocator(&allocatorCreateInfo, &vkAllocator));

    // Window
    SDL_Window* window = SDL_CreateWindow(
        createInfo.appName,
        1920u, 1080u,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    chk(SDL_Vulkan_CreateSurface(window, vkInstance, nullptr, &vkSurface));

    VkSurfaceCapabilitiesKHR surfaceCaps{};
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], vkSurface, &surfaceCaps));

    // Swapchain
    

}

void Application::MainLoop()
{

}

void Application::CleanUP()
{

}