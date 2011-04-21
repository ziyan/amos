player_add_plugin_driver (amosplanner
	SOURCES
		planner.h
		planner.cc
		astar.h
		astar.cc
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		map
		thread
		astar
)

install(TARGETS amosplanner
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
