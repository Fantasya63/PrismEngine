#pragma once

#include <vma/vk_mem_alloc.h>


#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif


struct AppCreateInfo
{
    const char* appName;
    int argc;
    const char* const* argv;
};

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
    VkSurfaceKHR vkSurface{ VK_NULL_HANDLE };

};