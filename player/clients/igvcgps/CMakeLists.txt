player_add_playercpp_client (amosigvcgps SOURCES
	igvcgps.cpp
	tsp.h
	tsp.cpp

	INCLUDEDIRS ${COMMON_DIR}
	LIBDIRS ${LIBRARY_OUTPUT_PATH}
	LINKLIBS map astar thread
)

install(TARGETS amosigvcgps
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

