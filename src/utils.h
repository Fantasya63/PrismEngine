#pragma once
#include <string_view>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

static inline void chk(VkResult result) {
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}
}

// static inline void chkSwapchain(VkResult result) {
// 	if (result < VK_SUCCESS) {
// 		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
// 			updateSwapchain = true;
// 			return;
// 		}
// 		std::cerr << "Vulkan call returned an error (" << result << ")\n";
// 		exit(result);
// 	}
//}

static inline void chk(bool result) {
	if (!result) {
		std::cerr << "Call returned an error\n";
		exit(result);
	}
}

static inline void Print(std::string_view message)
{
	std::cout << message << '\n';
}