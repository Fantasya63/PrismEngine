#pragma once
<<<<<<< HEAD

=======
>>>>>>> 6b910a2317cdb9315359c4fb432ed56412077df3
#include <iostream>
#include <string_view>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#ifndef NDEBUG
    // Debug mode: stringify the expression (#res) and capture function, file, and line
    #define chk(res) chk_impl((res), #res, __func__, __FILE__, __LINE__)
    #define chkSwapchain(res, pUpdate) chkSwapchain_impl((res), #res, pUpdate, __func__, __FILE__, __LINE__)
#else
    // Release mode: pass null/zero values
    #define chk(res) chk_impl((res), nullptr, nullptr, nullptr, 0)
    #define chkSwapchain(res, pUpdate) chkSwapchain_impl((res), nullptr, pUpdate, nullptr, nullptr, 0)
#endif

// --- Internal Implementation Functions ---

static inline void chk_impl(VkResult result, const char* codeStr, const char* funcName, const char* fileName, int line) {
    if (result != VK_SUCCESS) {
        std::cerr << "Vulkan call failed (" << result << ")\n";
        if (codeStr) {
            std::cerr << "  Code:     " << codeStr << "\n"
                      << "  Location: " << funcName << "() [" << fileName << ":" << line << "]\n";
        }
        std::exit(result);
    }
}

static inline void chk_impl(bool result, const char* codeStr, const char* funcName, const char* fileName, int line) {
    if (!result) {
        std::cerr << "Call failed\n";
        if (codeStr) {
            std::cerr << "  Code:     " << codeStr << "\n"
                      << "  Location: " << funcName << "() [" << fileName << ":" << line << "]\n";
        }
        std::exit(EXIT_FAILURE);
    }
}

static inline void chkSwapchain_impl(VkResult result, const char* codeStr, bool* pUpdateSwapChain, const char* funcName, const char* fileName, int line) {
    if (result < VK_SUCCESS) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            if (pUpdateSwapChain) {
                *pUpdateSwapChain = true;
            }
            return;
        }
        std::cerr << "Vulkan swapchain call failed (" << result << ")\n";
        if (codeStr) {
            std::cerr << "  Code:     " << codeStr << "\n"
                      << "  Location: " << funcName << "() [" << fileName << ":" << line << "]\n";
        }
        std::exit(result);
    }
}

VkFormat findSupportedDepthFormat(VkPhysicalDevice physicalDevice) {
    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT, 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

static inline void Print(std::string_view message)
{
	std::cout << message << '\n';
}

