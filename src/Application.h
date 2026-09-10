#pragma once

#include <vma/vk_mem_alloc.h>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>


#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

constexpr uint32_t maxFramesInFlight{ 2 };

struct AppWindowInfo
{
    uint32_t Width;
    uint32_t Height;

};

struct AppCreateInfo
{
    AppWindowInfo WindowInfo;
    const char* AppName;
    int Argc;
    const char* const* Argv;
};

typedef ShaderDataBuffer;
typedef VkSemaphore;
typedef Texture;

class Application
{
public:
    Application(const AppCreateInfo& info);
    void Run();
   
private:
    void InitSDL();
    void InitVulkan();
    void MainLoop();
    void CleanUP();

private:
    AppCreateInfo createInfo;

    VkInstance vkInstance;
    VkDevice vkDevice{ VK_NULL_HANDLE };
    VkQueue vkQueue{ VK_NULL_HANDLE };
    VmaAllocator vkAllocator{ VK_NULL_HANDLE };
    VmaAllocation vkDepthImageAllocation;

    VkSurfaceKHR vkSurface{ VK_NULL_HANDLE };
    VkSwapchainKHR vkSwapChain { VK_NULL_HANDLE };
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkImage vkDepthImage;
    VkImageView vkDepthImageView;

    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkPipeline pipeline{ VK_NULL_HANDLE };

    // Model
    VkBuffer modelVBuffer{ VK_NULL_HANDLE };
    VmaAllocation modelVBufferAllocation{ VK_NULL_HANDLE };
    
    std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
    std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers; 

    std::array<VkFence, maxFramesInFlight> fences;
    std::array<VkSemaphore, maxFramesInFlight> imageAcquiredSemaphores;

    std::vector<VkSemaphore> renderCompleteSemaphores;
    VkCommandPool vkCommandPool{ VK_NULL_HANDLE };

    std::array<VkCommandBuffer, maxFramesInFlight> vkCommandBuffers;

    // Textures
    std::array<Texture, 3> textures {};

    VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSetTex{ VK_NULL_HANDLE };

    // Slang
    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;

    uint32_t frameIndex{ 0 };
};