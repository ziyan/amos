include (UseOpenCV)

player_add_plugin_driver (amoscamera
	SOURCES
		camera.h
		camera.cc
	INCLUDEDIRS
		${OPENCV_INCLUDE_DIRS}
	LIBDIRS
		${OPENCV_LINK_DIRS}
	LINKLIBS
		${OPENCV_LINK_LIBS}
)

install(TARGETS amoscamera
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

