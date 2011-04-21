player_add_plugin_driver (amoslaserfusion SOURCES
	laserfusion.h
	laserfusion.cc
)

install(TARGETS amoslaserfusion
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
