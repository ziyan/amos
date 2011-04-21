player_add_plugin_driver (amosgps
	SOURCES
		gps.h
		gps.cc
		nmea.h
		nmea.c
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		serial
		utm
)

install(TARGETS amosgps
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

