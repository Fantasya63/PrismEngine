#pragma once
#include <cstdint>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

typedef uint32_t meshIndex_t;

// Compile-time helper function
constexpr VkIndexType getVkIndexType() {
    static_assert(
        std::is_same_v<meshIndex_t, uint16_t> ||
        std::is_same_v<meshIndex_t, uint32_t> ||
        std::is_same_v<meshIndex_t, uint8_t>,
        "meshIndex_t must be uint8_t, uint16_t, or uint32_t for Vulkan index buffers!"
        );

    if constexpr (std::is_same_v<meshIndex_t, uint16_t>) {
        return VK_INDEX_TYPE_UINT16;
    }
    else if constexpr (std::is_same_v<meshIndex_t, uint32_t>) {
        return VK_INDEX_TYPE_UINT32;
    }
    else if constexpr (std::is_same_v<meshIndex_t, uint8_t>) {
        // Requires VK_EXT_index_type_uint8 extension
        return VK_INDEX_TYPE_UINT8_EXT;
    }
}