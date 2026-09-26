#pragma once

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

VkFormat findSupportedDepthFormat(VkPhysicalDevice physicalDevice);

static inline void Print(std::string_view message)
{
	std::cout << message << '\n';
}

