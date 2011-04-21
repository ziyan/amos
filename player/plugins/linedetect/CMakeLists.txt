include (UseOpenCV)

player_add_plugin_driver (amoslinedetect
	SOURCES
		linetrim.cc
		linetrim.h
		linedetect.cc
		linedetect.h
		routines.cpp
		routines.h
		camerainfo.h
	INCLUDEDIRS
		${OPENCV_INCLUDE_DIRS}
	LIBDIRS
		${OPENCV_LINK_DIRS}
	LINKLIBS
		${OPENCV_LINK_LIBS}
)

install(TARGETS amoslinedetect
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
