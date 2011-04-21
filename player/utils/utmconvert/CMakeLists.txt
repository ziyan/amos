include_directories (${COMMON_DIR})
link_directories (${LIBRARY_OUTPUT_PATH})

add_executable (amosutmconvert
	utmconvert.cpp
)
target_link_libraries (amosutmconvert utm)

install(TARGETS amosutmconvert
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

