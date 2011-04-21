add_library (utm STATIC
	utm.h
	utm.c
)

set_target_properties(utm PROPERTIES COMPILE_FLAGS -fPIC)

target_link_libraries (utm m)


