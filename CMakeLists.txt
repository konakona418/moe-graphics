cmake_minimum_required(VERSION 3.10.0)
project(moe-graphics VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendors/glfw)
add_subdirectory(vendors/glm)
add_subdirectory(vendors/Vulkan-Headers)
add_subdirectory(vendors/vk-bootstrap)

set(VULKAN_HEADERS_INSTALL_DIR ${PROJECT_SOURCE_DIR}/vendors/Vulkan-Headers)
add_subdirectory(vendors/VulkanMemoryAllocator)

add_subdirectory(vendors/volk)

add_subdirectory(vendors/fmt)
set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(vendors/spdlog)

add_subdirectory(vendors/tinygltf)

add_subdirectory(vendors/JoltPhysics/Build)

add_subdirectory(vendors/harfbuzz)
add_subdirectory(vendors/freetype)
add_subdirectory(vendors/utfcpp)
add_subdirectory(vendors/openal-soft)

include_directories(include)

file(GLOB_RECURSE RENDER_SOURCES src/Render/*.cpp)
file(GLOB_RECURSE CORE_SOURCES src/Core/*.cpp)
file(GLOB_RECURSE MATH_SOURCES src/Math/*.cpp)
file(GLOB_RECURSE AUDIO_SOURCES src/Audio/*.cpp)
file(GLOB_RECURSE PHYSICS_SOURCES src/Physics/*.cpp)

file(GLOB IMGUI_SOURCES 
  vendors/imgui/imgui*.h 
  vendors/imgui/imgui*.cpp 
  vendors/imgui/backends/imgui_impl_glfw.*
  vendors/imgui/backends/imgui_impl_vulkan.*)

file(GLOB IM3D_SOURCES vendors/im3d/im3d.cpp)

file(GLOB STB_IMAGES_SOURCES 
  vendors/stb/stb_image.h
)

add_executable(moe-graphics 
  main.cpp
  ${RENDER_SOURCES}
  ${CORE_SOURCES}
  ${MATH_SOURCES}
  ${AUDIO_SOURCES}
  ${PHYSICS_SOURCES}

  ${IMGUI_SOURCES}
  ${IM3D_SOURCES}
  ${STB_IMAGES_SOURCES}
)

target_link_libraries(moe-graphics PRIVATE
  glfw glm::glm
  vk-bootstrap::vk-bootstrap VulkanMemoryAllocator volk_headers
  spdlog::spdlog fmt::fmt
  tinygltf
  Jolt
  utf8cpp freetype harfbuzz
  OpenAL::OpenAL
)

target_include_directories(moe-graphics PRIVATE
  vendors/span/include
  vendors/imgui
  vendors/imgui/backends
  vendors/im3d
  vendors/stb
)

#find_package(Vulkan REQUIRED)
#target_link_libraries(moe-graphics PRIVATE ${Vulkan_LIBRARIES})

target_compile_definitions(moe-graphics PRIVATE 
  VK_NO_PROTOTYPES
  IMGUI_IMPL_VULKAN_USE_VOLK # for imgui volk integration
)

include(CTest)
enable_testing()

