player_add_plugin_driver (amosgf SOURCES
	gf.h
	gf.cc
)

install(TARGETS amosgf
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
