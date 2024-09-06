set(RENDER_SOURCES 
    "src/instance_manager.cpp"  "include/instance_manager.hpp"
    "src/exceptions.cpp"        "include/exceptions.hpp"
    "include/version.hpp"
    "src/vk_result.cpp"         "src/vk_result.hpp"
    "src/device_manager.cpp"    "include/device_manager.hpp"
    "src/window.cpp"            "include/window.hpp"
    "src/logger.cpp"            "src/logger.hpp"
    "src/render_manager.cpp"    "include/render_manager.hpp")

add_library(render ${RENDER_SOURCES})

add_dependencies(render Bintool)

execute_process(COMMAND "${BINTOOL}" "${CMAKE_CURRENT_SOURCE_DIR}/frag.spv" "-l" OUTPUT_VARIABLE FRAG_SHADER)
execute_process(COMMAND "${BINTOOL}" "${CMAKE_CURRENT_SOURCE_DIR}/vert.spv" "-l" OUTPUT_VARIABLE VERT_SHADER)

configure_file("cfg/shaders.hpp" "cfg/shaders.hpp")

target_include_directories(render PUBLIC "include" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/cfg")
target_link_libraries(render PRIVATE Vulkan::Vulkan PUBLIC glfw spdlog::spdlog fmt::fmt)

target_compile_definitions(render PRIVATE
    ENGINE_NAME="${GRT_ENGINE_NAME}"
    ENGINE_VERSION_MAJOR=${GRT_VERSION_MAJOR}
    ENGINE_VERSION_MINOR=${GRT_VERSION_MINOR}
    ENGINE_VERSION_PATCH=${GRT_VERSION_PATCH}
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)