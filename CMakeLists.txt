cmake_minimum_required(VERSION 3.30)

project(GRT VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GRT_ENGINE_NAME "Game Runtime")

add_compile_definitions(DEBUG_ASSERTIONS=${DEBUG}
						TERMINAL_ENABLED=${TERMINAL}
						RADIANS=true)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)

set(TOOLS_DIR "${CMAKE_BINARY_DIR}/../tools")

if (WIN32)
	set(BINTOOL "${TOOLS_DIR}/bin/bintool.exe")
	set(COMPSHDR "${TOOLS_DIR}/bin/compshdr.exe")
elseif(UNIX)
	set(BINTOOL "${TOOLS_DIR}/bin/bintool")
	set(COMPSHDR "${TOOLS_DIR}/bin/compshdr")
endif()

execute_process(COMMAND
				"cargo" "install" "--path" "${GRT_SOURCE_DIR}/tools/bintool" "--root" "${TOOLS_DIR}"
)

execute_process(COMMAND
				"cargo" "install" "--path" "${GRT_SOURCE_DIR}/tools/compshdr" "--root" "${TOOLS_DIR}"
)

add_subdirectory(engine)
add_subdirectory(runtime)

add_custom_target(copy_compile_commands "cp" "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}/..")
execute_process(COMMAND "cp" "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}/..")