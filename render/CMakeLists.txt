set(RENDER_SOURCES
    "src/backend/instance_manager.cpp"   "src/backend/instance_manager.hpp"
    "src/exceptions.cpp"                 "include/exceptions.hpp"
    "include/version.hpp"
    "src/backend/vk_result.cpp"          "src/backend/vk_result.hpp"
    "src/backend/device_manager.cpp"     "src/backend/device_manager.hpp"
    "src/window.cpp"                     "include/window.hpp"
    "src/logger.cpp"                     "src/logger.hpp"
    "src/backend/vulkan_backend.cpp"     "src/backend/vulkan_backend.hpp"
    "include/vertex.hpp"                 "src/backend/vertex_description.hpp"
    "include/render_backend.hpp"         "src/backend/vma_impl.cpp"
    "include/object.hpp"
    "include/constants.hpp"
    "src/drawables/DrawingContext.hpp" 
    "src/drawables/GouraudMesh.cpp"      "src/drawables/GouraudMesh.hpp")

add_library(render ${RENDER_SOURCES})

add_dependencies(render CompSHDR)

execute_process(COMMAND "${COMPSHDR}" "${CMAKE_CURRENT_SOURCE_DIR}/shader.frag" "-m" "u32-list" "-k" "fragment" OUTPUT_VARIABLE FRAG_SHADER)
execute_process(COMMAND "${COMPSHDR}" "${CMAKE_CURRENT_SOURCE_DIR}/shader.vert" "-m" "u32-list" "-k" "vertex" OUTPUT_VARIABLE VERT_SHADER)

configure_file("cfg/shaders.hpp" "cfg/shaders.hpp")

target_include_directories(render PUBLIC "include" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/cfg")
target_link_libraries(render PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator PUBLIC glfw spdlog::spdlog fmt::fmt glm::glm)

target_compile_definitions(render PRIVATE
    ENGINE_NAME="${GRT_ENGINE_NAME}"
    ENGINE_VERSION_MAJOR=${GRT_VERSION_MAJOR}
    ENGINE_VERSION_MINOR=${GRT_VERSION_MINOR}
    ENGINE_VERSION_PATCH=${GRT_VERSION_PATCH}
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VMA_VULKAN_VERSION=1003000
)
