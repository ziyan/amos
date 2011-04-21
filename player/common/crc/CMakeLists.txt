add_library (crc STATIC
	crc.h
	crc.c
)

set_target_properties(crc PROPERTIES COMPILE_FLAGS -fPIC)
