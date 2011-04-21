player_add_plugin_driver (amosbase
	SOURCES
		base.h
		base.cc
		packet.h
		comm.h
		comm.c
	INCLUDEDIRS
		${COMMON_DIR}
	LIBDIRS
		${LIBRARY_OUTPUT_PATH}
	LINKLIBS
		serial
		crc
)

install(TARGETS amosbase
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

