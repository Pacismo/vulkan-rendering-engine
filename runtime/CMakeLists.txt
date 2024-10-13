set(RUNTIME_SOURCES 
	"main.cpp" 
	"HintBox.hpp"       "HintBox.cpp" 
	"ObjectMutator.hpp" "ObjectMutator.cpp"
	"Cube.hpp"          "Cube.cpp"
)

if (${TERMINAL} OR NOT (WIN32 AND MSVC))
add_executable(runtime ${RUNTIME_SOURCES})
else()
add_executable(runtime WIN32 ${RUNTIME_SOURCES})
endif ()

target_link_libraries(runtime PRIVATE engine)
