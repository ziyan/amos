include (UseMemcached)

include_directories (${MEMCACHED_INCLUDE_DIRS})
link_directories (${MEMCACHED_LINK_DIRS})

add_library (map STATIC
	map.h
	map.cc
	memcached.h
	define.h
	type.h
	type.cc
)

set_target_properties(map PROPERTIES COMPILE_FLAGS -fPIC)

target_link_libraries (map ${MEMCACHED_LINK_LIBS})



