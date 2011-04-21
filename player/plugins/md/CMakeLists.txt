player_add_plugin_driver (amosmd SOURCES
	md.h
	md.cc
	WMM.h
	WMM.cc
)

install(TARGETS amosmd
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
