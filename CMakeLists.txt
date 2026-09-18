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


# VULKAN
if(DEFINED ENV{VULKAN_SDK})
    file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_DIR)
    set(Vulkan_ROOT "${VULKAN_SDK_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_DIR}")
endif()

find_package(Vulkan REQUIRED)

# Find Volk in Vulkan SDK
find_path(VOLK_INCLUDE_DIR NAMES volk.h HINTS $ENV{VULKAN_SDK}/include)
find_library(VOLK_LIBRARY NAMES volk HINTS $ENV{VULKAN_SDK}/lib $ENV{VULKAN_SDK}/Lib)

add_library(volk INTERFACE)
target_include_directories(volk INTERFACE ${VOLK_INCLUDE_DIR})
target_link_libraries(volk INTERFACE ${VOLK_LIBRARY})


add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)


# SLANG
find_path(SLANG_INCLUDE_DIR 
    NAMES slang/slang.h slang.h
    HINTS 
        $ENV{VULKAN_SDK}/include
        ${Vulkan_INCLUDE_DIRS}
)

find_library(SLANG_LIBRARY 
    NAMES slang slang-lib
    HINTS 
        $ENV{VULKAN_SDK}/lib
        $ENV{VULKAN_SDK}/Lib
        ${Vulkan_LIBRARIES}
)

if(NOT SLANG_INCLUDE_DIR OR NOT SLANG_LIBRARY)
    message(FATAL_ERROR "Slang was not found in Vulkan SDK!\nChecked SDK Path: $ENV{VULKAN_SDK}")
endif()

add_library(Slang::Slang INTERFACE IMPORTED)
target_include_directories(Slang::Slang INTERFACE ${SLANG_INCLUDE_DIR})
target_link_libraries(Slang::Slang INTERFACE ${SLANG_LIBRARY})


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
    Vulkan::volk
    glm::glm
    SDL3::SDL3-static # Links the static target provided by FetchContent
    ktx
    Slang::Slang
)


