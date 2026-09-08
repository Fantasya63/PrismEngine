cmake_minimum_required(VERSION 3.29)
project(PrismEngine VERSION 0.1.0 LANGUAGES C CXX)

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
    KTX
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


find_package(glm REQUIRED)
find_package(Vulkan 1.4.335 REQUIRED)

include_directories(external/tinyobj)


add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
target_compile_definitions(VulkanCppModule
    INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)


add_executable(PrismEngine
    src/main.cpp
    src/Application.cpp
)
set_target_properties(PrismEngine PROPERTIES CXX_STANDARD 20)

target_compile_definitions(PrismEngine PRIVATE VULKAN_HPP_NO_EXCEPTIONS)
target_compile_definitions(PrismEngine PRIVATE "VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS")


target_link_libraries(PrismEngine PRIVATE 
    Vulkan::cppm
    glm::glm
    SDL3::SDL3-static # Links the static target provided by FetchContent
    KTX
)

# Expose KTX and local headers cleanly
target_include_directories(PrismEngine PRIVATE
    ${ktx_SOURCE_DIR}/include
    ${ktx_SOURCE_DIR}/other_include
)

