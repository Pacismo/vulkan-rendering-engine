set(ENGINE_SOURCES
    "shader.vert"
    "shader.frag"

    "include/version.hpp"
    "src/object.cpp"                            "include/object.hpp"
    "include/constants.hpp"
    "include/vertex.hpp"
    "include/input/keyboard.hpp"
    "include/input/mouse.hpp"
    "include/input/controller.hpp"

    "include/reflection/datastructure.hpp"
    "include/reflection/fieldtype.hpp"

    "src/logger.cpp"                             "include/logger.hpp"
    "src/window.cpp"                             "include/window.hpp"
    "src/exceptions.cpp"                         "include/exceptions.hpp"

    "include/resources/image.hpp"

    "src/backend/vma_impl.cpp"
    "include/backend/vertex_description.hpp"
    "include/backend/pipeline_configuration.hpp"
    "include/backend/allocation.hpp"
    "src/backend/image_allocation.cpp"           "include/backend/image_allocation.hpp"
    "src/backend/instance_manager.cpp"           "include/backend/instance_manager.hpp"
    "src/backend/vk_result.cpp"                  "include/backend/vk_result.hpp"
    "src/backend/device_manager.cpp"             "include/backend/device_manager.hpp"
    "src/backend/vulkan_backend.cpp"             "include/backend/vulkan_backend.hpp"
    "src/backend/descriptor_pool.cpp"            "include/backend/descriptor_pool.hpp"
    "src/backend/command_pool.cpp"               "include/backend/command_pool.hpp"
    "src/backend/allocator.cpp"                  "include/backend/allocator.hpp"
    "src/backend/swapchain.cpp"                  "include/backend/swapchain.hpp"

    "src/gui/imgui_manager.cpp"                  "include/gui/imgui_manager.hpp"
    "src/gui/applet.cpp"                         "include/gui/applet.hpp"

    "include/drawables/drawing_context.hpp"
    "src/drawables/GouraudMesh.cpp"              "include/drawables/GouraudMesh.hpp"
)

add_library(engine ${ENGINE_SOURCES})

execute_process(COMMAND "${COMPSHDR}" "${CMAKE_CURRENT_SOURCE_DIR}/shader.frag" "-m" "u32-list" "-k" "fragment" OUTPUT_VARIABLE FRAG_SHADER)
execute_process(COMMAND "${COMPSHDR}" "${CMAKE_CURRENT_SOURCE_DIR}/shader.vert" "-m" "u32-list" "-k" "vertex" OUTPUT_VARIABLE VERT_SHADER)

configure_file("cfg/shaders.hpp" "cfg/shaders.hpp")

target_include_directories(engine PUBLIC "include" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/cfg")
target_link_libraries(engine PUBLIC Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator glfw spdlog::spdlog fmt::fmt glm::glm imgui::imgui)

target_compile_definitions(engine
PRIVATE
    ENGINE_NAME="${GRT_ENGINE_NAME}"
    ENGINE_VERSION_MAJOR=${GRT_VERSION_MAJOR}
    ENGINE_VERSION_MINOR=${GRT_VERSION_MINOR}
    ENGINE_VERSION_PATCH=${GRT_VERSION_PATCH}
PUBLIC
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VMA_VULKAN_VERSION=1003000
)
