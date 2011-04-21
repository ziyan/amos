player_add_plugin_driver (amosmapper
	SOURCES
		mapper.h
		mapper.cc
		elevation.cc
		visual.cc
		probability.cc
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		map
)

INSTALL(TARGETS amosmapper
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

