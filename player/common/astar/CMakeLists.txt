include(UsePlayerPlugin)
include_directories(${COMMON_DIR} ${PLAYERCORE_INCLUDE_DIRS})
link_directories (${PLAYERCORE_LINK_DIRS} ${LIBRARY_OUTPUT_PATH})

add_library (astar STATIC
	astar.h
	astar.cc
)

set_target_properties(astar PROPERTIES COMPILE_FLAGS "-fPIC -std=c++0x")
target_link_libraries(astar -lmap ${PLAYERCORE_LINK_LIBS})

