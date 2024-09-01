add_library(render 
    "src/instance_manager.cpp"  "include/instance_manager.hpp"
    "src/exceptions.cpp"        "include/exceptions.hpp"
    "include/version.hpp"
    "src/vk_result.cpp"         "src/vk_result.hpp"
    "src/device_manager.cpp"    "include/device_manager.hpp")

target_include_directories(render PUBLIC "include")
target_link_libraries(render PRIVATE Vulkan::Vulkan PUBLIC glfw spdlog::spdlog fmt::fmt)

target_compile_definitions(render PRIVATE
    ENGINE_NAME="${GRT_ENGINE_NAME}"
    ENGINE_VERSION_MAJOR=${GRT_VERSION_MAJOR}
    ENGINE_VERSION_MINOR=${GRT_VERSION_MINOR}
    ENGINE_VERSION_PATCH=${GRT_VERSION_PATCH}
    VULKAN_HPP_NO_EXCEPTIONS=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)
