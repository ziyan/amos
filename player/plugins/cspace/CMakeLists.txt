player_add_plugin_driver (amoscspace
	SOURCES
		cspace.h
		cspace.cc
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		map
)

install(TARGETS amoscspace
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

