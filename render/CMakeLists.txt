find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(render 
    "src/instance_manager.cpp" "include/instance_manager.hpp"
    "src/exceptions.cpp" "include/exceptions.hpp"
    "include/version.hpp"
    "src/vk_result_to_string.cpp" "src/vk_result_to_string.hpp")

target_include_directories(render PUBLIC "include")
target_link_libraries(render PRIVATE Vulkan::Vulkan PUBLIC glfw)

target_compile_definitions(render PRIVATE
    ENGINE_NAME="${GRT_ENGINE_NAME}"
    ENGINE_VERSION_MAJOR=${GRT_VERSION_MAJOR}
    ENGINE_VERSION_MINOR=${GRT_VERSION_MINOR}
    ENGINE_VERSION_PATCH=${GRT_VERSION_PATCH}
    VULKAN_HPP_NO_EXCEPTIONS=1
)
