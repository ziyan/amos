include (UseOpenCV)

player_add_playercpp_client (amoscamviewer
	SOURCES camviewer.cpp
	INCLUDEDIRS ${OPENCV_INCLUDE_DIRS}
	LIBDIRS ${OPENCV_LINK_DIRS}
	LINKLIBS ${OPENCV_LINK_LIBS}
)

install(TARGETS amoscamviewer
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

