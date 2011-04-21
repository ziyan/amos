player_add_plugin_driver (amoscompass
	SOURCES
		compass.h
		compass.cc
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		serial
)

install(TARGETS amoscompass
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

