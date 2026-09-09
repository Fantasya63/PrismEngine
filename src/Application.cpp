#include "Application.h"
#include "utils.h"

#include <filesystem>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <ktx.h>
#include <ktxvulkan.h>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

const std::filesystem::path shaderSourcePath = "assets/shader.slang";

struct Texture {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

typedef uint16_t meshIndex_t; 


struct ShaderData {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model[3];
    glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
    uint32_t selected{1};
} shaderData{};

struct ShaderDataBuffer {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VmaAllocationInfo allocationInfo{};
	VkBuffer buffer{ VK_NULL_HANDLE };
	VkDeviceAddress deviceAddress{};
};

Application::Application(const AppCreateInfo& info)
    : createInfo(info),
    vkInstance(VK_NULL_HANDLE),
    vkDevice(VK_NULL_HANDLE),
    vkAllocator(VK_NULL_HANDLE),
    vkSurface(VK_NULL_HANDLE),
    vkSwapChain(VK_NULL_HANDLE),
    modelVBuffer(VK_NULL_HANDLE),
    modelVBufferAllocation(VK_NULL_HANDLE),
    vkCommandPool(VK_NULL_HANDLE),
    textures({}),
    descriptorSetLayoutTex(VK_NULL_HANDLE),
    descriptorPool( VK_NULL_HANDLE ),
    descriptorSetTex( VK_NULL_HANDLE ),
    pipelineLayout( VK_NULL_HANDLE ),
    pipeline( VK_NULL_HANDLE )
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
        .pApplicationName = createInfo.AppName,
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
    if (createInfo.Argc > 1)
    {
        deviceIndex = std::atoi(createInfo.Argv[1]);
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
        createInfo.AppName,
        createInfo.WindowInfo.Width, createInfo.WindowInfo.Height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    chk(SDL_Vulkan_CreateSurface(window, vkInstance, nullptr, &vkSurface));

    VkSurfaceCapabilitiesKHR surfaceCaps{};
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], vkSurface, &surfaceCaps));

    // Swapchain
    VkExtent2D swapChainExtent { surfaceCaps.currentExtent };
    if (swapChainExtent.width = 0xFFFFFFFF)
    {
        swapChainExtent = {
            .width = static_cast<uint32_t>(createInfo.WindowInfo.Width),
            .height = static_cast<uint32_t>(createInfo.WindowInfo.Height)
        };
    }

    const VkFormat imageFormat { VK_FORMAT_B8G8R8A8_SRGB };
    VkSwapchainCreateInfoKHR swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent {.width = swapChainExtent.width, .height = swapChainExtent.height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    chk(vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &vkSwapChain));

    uint32_t swapChainImageCount {0};
    chk(vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &swapChainImageCount, nullptr));
    swapchainImages.resize(swapChainImageCount);
    chk(vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &swapChainImageCount, swapchainImages.data()));
    swapchainImageViews.resize(swapChainImageCount);

    std::vector<VkFormat> depthFormatList { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    VkFormat depthFormat { VK_FORMAT_UNDEFINED };
    for (VkFormat& format : depthFormatList)
    {
        VkFormatProperties2 formatProperties {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
        if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            depthFormat = format;
            break;
        }
    }

    VkImageCreateInfo depthImageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent {.width = static_cast<uint32_t>(createInfo.WindowInfo.Width), .height = static_cast<uint32_t>(createInfo.WindowInfo.Height)},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo allocCreateInfo {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    chk(vmaCreateImage(vkAllocator, &depthImageCreateInfo, &allocCreateInfo, &vkDepthImage, &vkDepthImageAllocation, nullptr));

    VkImageViewCreateInfo depthImageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vkDepthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
    };
    chk(vkCreateImageView(vkDevice, &depthImageViewCreateInfo, nullptr, &vkDepthImageView));

    tinyobj::attrib_t tinyObjAttrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    chk(tinyobj::LoadObj(&tinyObjAttrib, &shapes, &materials, nullptr, nullptr, "asses/suzanne.obj"));

    const VkDeviceSize indexCount {shapes[0].mesh.indices.size()};
    std::vector<Vertex> vertices{};
    std::vector<meshIndex_t> indices{};

    for (auto& index : shapes[0].mesh.indices)
    {
        Vertex v {
            .pos = { tinyObjAttrib.vertices[index.vertex_index * 3], -tinyObjAttrib.vertices[index.vertex_index * 3 + 1], tinyObjAttrib.vertices[index.vertex_index * 3 + 2] },
            .normal = { tinyObjAttrib.normals[index.normal_index * 3], -tinyObjAttrib.normals[index.normal_index * 3 + 1], tinyObjAttrib.normals[index.normal_index * 3 + 2]},
            .uv = { tinyObjAttrib.texcoords[index.texcoord_index * 2], 1.0f - tinyObjAttrib.texcoords[index.texcoord_index * 2 + 1]}
        };

        vertices.push_back(v);
        indices.push_back(static_cast<meshIndex_t>(indices.size()));
    }

    // Upload model data to gpu
    VkDeviceSize vertexBufferSize { sizeof(Vertex) * vertices.size() };
    VkDeviceSize indexBufferSize {sizeof(meshIndex_t) * indices.size()};
    VkBufferCreateInfo modelBufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertexBufferSize + indexBufferSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };

    VmaAllocationCreateInfo vertexBufferAllocCreateInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VmaAllocationInfo vertexBufferAllocInfo{};
    chk(vmaCreateBuffer(vkAllocator, &modelBufferCreateInfo, &vertexBufferAllocCreateInfo, &modelVBuffer, &modelVBufferAllocation, &vertexBufferAllocInfo));

    memcpy(vertexBufferAllocInfo.pMappedData, vertices.data(), vertexBufferSize);
    memcpy(((char*)vertexBufferAllocInfo.pMappedData) + vertexBufferSize, indices.data(), indexBufferSize);

    // Allocate Buffers for ping pong buffers for frames in flight
    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        VkBufferCreateInfo bufferCI {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(ShaderData),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        VmaAllocationCreateInfo uBufferAllocInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        chk(vmaCreateBuffer(vkAllocator, &bufferCI, &uBufferAllocInfo, &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo));

        VkBufferDeviceAddressInfo uBufferDeviceAddressInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shaderDataBuffers[i].buffer
        };
        shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(vkDevice, &uBufferDeviceAddressInfo);
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        chk(vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &fences[i]));
        chk(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &imageAcquiredSemaphores[i]));
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (auto& semaphore : renderCompleteSemaphores)
    {
        chk(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &semaphore));
    }

    // Command Buffers
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex
    };
    chk(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &vkCommandPool));

    VkCommandBufferAllocateInfo commandBufferAllocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .commandBufferCount = maxFramesInFlight
    };
    chk(vkAllocateCommandBuffers(vkDevice, &commandBufferAllocInfo, vkCommandBuffers.data()));

    // Textures
    std::vector<VkDescriptorImageInfo> textureDescriptors {};
    for (size_t i = 0; i < textures.size(); i++)
    {
        ktxTexture* ktxTexture{ nullptr };
        std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
        ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

        VkImageCreateInfo textureImageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ktxTexture_GetVkFormat(ktxTexture),
            .extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1},
            .mipLevels = ktxTexture->numLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        VmaAllocationCreateInfo textureAllocCreateInfo {.usage= VMA_MEMORY_USAGE_AUTO};
        chk(vmaCreateImage(vkAllocator, &textureImageCreateInfo, &textureAllocCreateInfo, &textures[i].image, &textures[i].allocation, nullptr));

        VkImageViewCreateInfo texImageViewCI {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = textures[i].image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = textureImageCreateInfo.format,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1}
        };
        chk(vkCreateImageView(vkDevice, &texImageViewCI, nullptr, &textures[i].view));

        // Upload Data
        VkBuffer imgSrcBuffer {};
        VmaAllocation imgSrcAlloc {};
        VkBufferCreateInfo imgSrcBufferCI {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = (uint32_t)ktxTexture->dataSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };
        VmaAllocationCreateInfo imgSrcAllocCI {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        VmaAllocationInfo imgSrcAllocInfo{};
        chk(vmaCreateBuffer(vkAllocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAlloc, &imgSrcAllocInfo));

        memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);

        VkFenceCreateInfo fenceCreateInfoOneTime {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };

        VkFence fenceOneTime {};
        chk(vkCreateFence(vkDevice, &fenceCreateInfoOneTime, nullptr, &fenceOneTime));

        VkCommandBuffer commandBufferOnetime {};
        VkCommandBufferAllocateInfo commandBufferAllocInfoOneTime {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vkCommandPool,
            .commandBufferCount = 1
        };
        chk(vkAllocateCommandBuffers(vkDevice, &commandBufferAllocInfo, &commandBufferOnetime));

        VkCommandBufferBeginInfo cbOneTimeBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        chk(vkBeginCommandBuffer(commandBufferOnetime, &cbOneTimeBeginInfo));

        VkImageMemoryBarrier2 barrierTexImage {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = textures[i].image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1}
        };
        VkDependencyInfo barrierTexInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrierTexImage
        };

        vkCmdPipelineBarrier2(commandBufferOnetime, &barrierTexInfo);
        std::vector<VkBufferImageCopy> copyRegions {};
        for (auto j = 0; j < ktxTexture->numLevels; j++)
        {
            ktx_size_t mipOffset{0};
            KTX_error_code retErrCode = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
            copyRegions.push_back({
                .bufferOffset = mipOffset,
                .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1 },
                .imageExtent {.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
            });
        }
        vkCmdCopyBufferToImage(commandBufferOnetime, imgSrcBuffer, textures[i].image,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
        
        VkImageMemoryBarrier2 barrierTexRead{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .image = textures[i].image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1}
        };

        barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
        vkCmdPipelineBarrier2(commandBufferOnetime, &barrierTexInfo);
        chk(vkEndCommandBuffer(commandBufferOnetime));
        
        VkCommandBufferSubmitInfo commandBuffSubmitInfoOnetime {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = commandBufferOnetime
        };

        VkSubmitInfo2 oneTimeSI {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBuffSubmitInfoOnetime
        };
        chk(vkQueueSubmit2(vkQueue, 1, &oneTimeSI, fenceOneTime));
        chk(vkWaitForFences(vkDevice, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
        vkDestroyFence(vkDevice, fenceOneTime, nullptr);
        vmaDestroyBuffer(vkAllocator, imgSrcBuffer, imgSrcAlloc);

        VkSamplerCreateInfo samplerCI {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = 8.0f,
            .maxLod = (float)ktxTexture->numLevels,
        };
        chk(vkCreateSampler(vkDevice, &samplerCI, nullptr, &textures[i].sampler));
        ktxTexture_Destroy(ktxTexture);
        textureDescriptors.push_back({
            .sampler = textures[i].sampler,
            .imageView = textures[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

    // Texture Descriptors
    VkDescriptorBindingFlags descVariableFlag { VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
    VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlagsCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &descVariableFlag
    };
    VkDescriptorSetLayoutBinding descriptorLayoutBindingTex {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textures.size()),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descBindingFlagsCI,
        .bindingCount = 1,
        .pBindings = &descriptorLayoutBindingTex
    };
    chk(vkCreateDescriptorSetLayout(vkDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayoutTex));

    // Descriptor Pool
    VkDescriptorPoolSize descriptorPoolSize {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textures.size())
    };
    VkDescriptorPoolCreateInfo descPoolCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize
    };
    chk(vkCreateDescriptorPool(vkDevice, &descPoolCI, nullptr, &descriptorPool));
    
    uint32_t variableDescCount { static_cast<uint32_t>(textures.size()) };
    VkDescriptorSetVariableDescriptorCountAllocateInfo descSetVarDescCountAllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &variableDescCount
    };
    VkDescriptorSetAllocateInfo texDescSetAlloc {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &descSetVarDescCountAllocInfo,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayoutTex
    };
    chk(vkAllocateDescriptorSets(vkDevice, &texDescSetAlloc, &descriptorSetTex));

    VkWriteDescriptorSet writeDescSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSetTex,
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };

    vkUpdateDescriptorSets(vkDevice, 1, &writeDescSet, 0, nullptr);

    // Init SlangShaderCompiler
    slang::createGlobalSession(slangGlobalSession.writeRef());

    auto slangTargets { std::to_array<slang::TargetDesc>(
        {
            {
                .format{SLANG_SPIRV},
                .profile{slangGlobalSession->findProfile("spirv_1_4")}
            }
        }
    )};

    auto slangOptions { std::to_array<slang::CompilerOptionEntry>(
        {
            {
                slang::CompilerOptionName::EmitSpirvDirectly,
                { slang::CompilerOptionValueKind::Int, 1 }
            }
        }
    )};

    slang::SessionDesc slangSessionDesc{
        .targets {slangTargets.data()},
        .targetCount {SlangInt(slangTargets.size())},
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .compilerOptionEntries {slangOptions.data()},
        .compilerOptionEntryCount {uint32_t (slangOptions.size())}
    };

    Slang::ComPtr<slang::ISession> slangSession;
    slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

    Slang::ComPtr<slang::IModule> slangModule {
        slangSession->loadModuleFromSourceString("triangle", shaderSourcePath.c_str(), nullptr, nullptr)
    };
    Slang::ComPtr<ISlangBlob> spirv;
    slangModule->getTargetCode(0, spirv.writeRef());

    VkShaderModuleCreateInfo shaderModuleCI {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = (uint32_t*)spirv->getBufferPointer()
    };
    VkShaderModule shaderModule{};
    chk(vkCreateShaderModule(vkDevice, &shaderModuleCI, nullptr, &shaderModule));

    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(VkDeviceAddress)
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayoutTex,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    chk(vkCreatePipelineLayout(vkDevice, &pipelineLayoutCI, nullptr, &pipelineLayout));

    VkVertexInputBindingDescription vertexInputBindingDesc {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> vertexInputAttributesDesc {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)},
        { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)},
    };

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDesc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributesDesc.size()),
        .pVertexAttributeDescriptions = vertexInputAttributesDesc.data()
    };

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs {
        { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaderModule, .pName = "main"
        },
        { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaderModule, .pName = "main"
        },
    };

    VkPipelineViewportStateCreateInfo pipelineViewportStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    std::vector<VkDynamicState> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    VkPipelineRenderingCreateInfo pipelineRenderingCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &imageFormat,
        .depthAttachmentFormat = depthFormat
    };

    VkPipelineColorBlendAttachmentState blendAttachmentState {
        .colorWriteMask = 0xF
    };

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentState
    };

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo pipelineMultiSampleStateCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCI {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCI,
        .stageCount = static_cast<uint32_t>(shaderStageCIs.size()),
        .pStages = shaderStageCIs.data(),
        .pVertexInputState = &pipelineVertexInputStateCI,
        .pInputAssemblyState = &pipelineInputAssemblyStateCI,
        .pViewportState = &pipelineViewportStateCI,
        .pRasterizationState = &pipelineRasterizationStateCI,
        .pMultisampleState = &pipelineMultiSampleStateCI,
        .pDepthStencilState = &pipelineDepthStencilStateCI,
        .pColorBlendState = &pipelineColorBlendStateCI,
        .pDynamicState = &pipelineDynamicStateCI,
        .layout = pipelineLayout
    };
    chk(vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &pipeline));
}

void Application::MainLoop()
{
    
}

void Application::CleanUP()
{

}