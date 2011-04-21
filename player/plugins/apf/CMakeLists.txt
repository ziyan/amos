player_add_plugin_driver (amosapf SOURCES
	apf.h
	apf.cc
)

install(TARGETS amosapf
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
