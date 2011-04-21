player_add_plugin_driver (amoslocalize SOURCES
	localize.h
	localize.cc
)

install(TARGETS amoslocalize
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
