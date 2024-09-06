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

set(TOOLS_DIR "${CMAKE_BINARY_DIR}/tools")

if (WIN32)
	set(BINTOOL "${TOOLS_DIR}/bin/bintool.exe")
elseif(UNIX)
	set(BINTOOL "${TOOLS_DIR}/bin/bintool")
endif()

add_custom_target(Bintool
				  "cargo" "install" "--path" "${GRT_SOURCE_DIR}/tools/bintool" "--root" "${TOOLS_DIR}"
				  BYPRODUCTS "${BINTOOL}"
				  SOURCES "${GRT_SOURCE_DIR}/tools/bintool/src/main.rs" "${GRT_SOURCE_DIR}/tools/bintool/src/error.rs"
)

add_subdirectory(render)
add_subdirectory(runtime)

add_custom_target(copy_compile_commands cp "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}/..")
