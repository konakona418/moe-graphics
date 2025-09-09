cmake_minimum_required(VERSION 3.10.0)
project(moe-graphics VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(USE_VULKAN_SDK ON)

add_subdirectory(vendors/glfw)
add_subdirectory(vendors/glm)

set(ENABLE_SPIRV OFF)
add_subdirectory(vendors/glslang)

set(VK_BOOTSTRAP_VULKAN_HEADER_DIR ${PROJECT_SOURCE_DIR}/vendors/Vulkan-Headers/include)
add_subdirectory(vendors/vk-bootstrap)

add_subdirectory(vendors/VulkanMemoryAllocator)

add_subdirectory(vendors/spdlog)

include_directories(include)

file(GLOB_RECURSE ENGINE_SOURCES src/Engine/*.cpp)
file(GLOB_RECURSE RENDER_SOURCES src/Render/*.cpp)
file(GLOB_RECURSE CORE_SOURCES src/Core/*.cpp)
file(GLOB_RECURSE PLUGIN_SOURCES plugins/*/*.cpp)

add_executable(moe-graphics 
  main.cpp
  ${ENGINE_SOURCES}
  ${RENDER_SOURCES}
  ${CORE_SOURCES}
  ${PLUGIN_SOURCES}
)

target_link_libraries(moe-graphics PUBLIC
  glfw glm::glm glslang 
  vk-bootstrap::vk-bootstrap VulkanMemoryAllocator
  spdlog::spdlog
)

if(USE_VULKAN_SDK)
  find_package(Vulkan REQUIRED)

  target_link_directories(moe-graphics PUBLIC ${Vulkan_LIBRARY_DIRS})
  target_link_libraries(moe-graphics PUBLIC ${Vulkan_LIBRARIES})
else()
  error("Currently only Vulkan SDK is supported.")
endif()

include(CTest)
enable_testing()

