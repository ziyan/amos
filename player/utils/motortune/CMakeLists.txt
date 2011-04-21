include_directories (${COMMON_DIR})
link_directories (${LIBRARY_OUTPUT_PATH})

add_executable (amosmotortune
	tune.c
	../../plugins/base/comm.h
	../../plugins/base/comm.c
)
target_link_libraries (amosmotortune serial crc)

install(TARGETS amosmotortune
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

