cmake_minimum_required(VERSION 3.29)
project(PrismEngine LANGUAGES C CXX)

include(FetchContent)


FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0  # Pins to a stable release
    GIT_SHALLOW    ON
)

# Configure SDL3 options before bringing it in
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Force static build
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)          # Disable unit tests to speed up build
set(SDL_TESTS OFF CACHE BOOL "" FORCE)         # Disable SDL3 test executables
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)  # Disable SDL3 test support library

FetchContent_MakeAvailable(SDL3)


# KTX
FetchContent_Declare(
    ktx
    GIT_REPOSITORY  https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG         v4.4.2
    GIT_SHALLOW     ON
)

set(KTX_FEATURE_STATIC_LIB ON CACHE BOOL "" FORCE)

set( KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_JNI OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_PY OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_TOOLS_CTS OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_JS OFF CACHE BOOL "" FORCE)
set( KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(KTX)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1          # Pins to a stable GLM version
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(glm)

# # 1. Normalize VULKAN_SDK environment variable for CMake
# if(DEFINED ENV{VULKAN_SDK})
#     file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_DIR)
#     set(Vulkan_ROOT "${VULKAN_SDK_DIR}")
# endif()

find_package(Vulkan REQUIRED)
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG        vulkan-sdk-1.3.296.0
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(volk)


add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)


# Specify the latest stable Slang release tag
set(SLANG_VERSION "v2026.18")
string(REGEX REPLACE "^v" "" SLANG_VER_NUM "${SLANG_VERSION}")

if(UNIX AND NOT APPLE)
    set(SLANG_ARCHIVE "slang-${SLANG_VER_NUM}-linux-x86_64.zip")
elseif(WIN32)
    set(SLANG_ARCHIVE "slang-${SLANG_VER_NUM}-win64.zip")
endif()

FetchContent_Declare(
    slang_binaries
    URL https://github.com/shader-slang/slang/releases/download/${SLANG_VERSION}/${SLANG_ARCHIVE}
)
FetchContent_MakeAvailable(slang_binaries)

add_library(Slang::Slang INTERFACE IMPORTED)

target_include_directories(Slang::Slang INTERFACE ${slang_binaries_SOURCE_DIR}/include)

if(UNIX AND NOT APPLE)
    target_link_libraries(Slang::Slang INTERFACE ${slang_binaries_SOURCE_DIR}/lib/libslang.so)
elseif(WIN32)
    target_link_libraries(Slang::Slang INTERFACE ${slang_binaries_SOURCE_DIR}/lib/slang.lib)
endif()




target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
target_compile_definitions(VulkanCppModule
    INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)


add_executable(PrismEngine
    src/main.cpp
    src/Application.cpp
)

if (MSVC)
    target_compile_options(PrismEngine PRIVATE /std:c++latest)
endif()

set_target_properties(PrismEngine PROPERTIES CXX_STANDARD 20)
target_compile_definitions(PrismEngine PRIVATE VULKAN_HPP_NO_EXCEPTIONS)
target_compile_definitions(PrismEngine PRIVATE "VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS")

# Expose KTX and local headers cleanly
target_include_directories(PrismEngine PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${ktx_SOURCE_DIR}/include
    ${ktx_SOURCE_DIR}/other_include
    
)

target_link_libraries(PrismEngine PRIVATE 
    Vulkan::cppm
    volk::volk
    glm::glm
    SDL3::SDL3-static # Links the static target provided by FetchContent
    ktx
    Slang::Slang
)


