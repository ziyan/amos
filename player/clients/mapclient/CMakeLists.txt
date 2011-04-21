player_add_playercpp_client (amosmapclient SOURCES
	mapclient.cpp

	INCLUDEDIRS ${COMMON_DIR}
	LIBDIRS ${LIBRARY_OUTPUT_PATH}
	LINKLIBS map
)

install(TARGETS amosmapclient
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

