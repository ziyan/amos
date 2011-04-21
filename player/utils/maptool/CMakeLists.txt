include (UseSqlite3)
include (UseMemcached)

include_directories (${SQLITE3_INCLUDE_DIRS} ${MEMCACHED_INCLUDE_DIRS} ${COMMON_DIR})
link_directories (${SQLITE3_LINK_DIRS} ${MEMCACHED_LINK_DIRS} ${LIBRARY_OUTPUT_PATH})

add_executable (amosmaptool
	maptool.h
	main.cpp
	db.cpp
	memcached.cpp
)
target_link_libraries (amosmaptool ${SQLITE3_LINK_LIBS} ${MEMCACHED_LINK_LIBS} map)

install(TARGETS amosmaptool
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

