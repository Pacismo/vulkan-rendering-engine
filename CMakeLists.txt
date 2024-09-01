cmake_minimum_required(VERSION 3.30)

project(GRT VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GRT_ENGINE_NAME "Game Runtime")

add_compile_definitions(DEBUG_ASSERTIONS=${DEBUG})

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)

add_subdirectory(render)
add_subdirectory(runtime)

add_custom_target(copy_compile_commands cp "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}/..")
