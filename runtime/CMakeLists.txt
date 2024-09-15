if (${TERMINAL} OR NOT WIN32)
add_executable(runtime "main.cpp")
else()
add_executable(runtime WIN32 "main.cpp")
endif ()

target_link_libraries(runtime PRIVATE render)