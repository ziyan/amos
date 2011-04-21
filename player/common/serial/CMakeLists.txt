add_library (serial STATIC
	serial.h
	serial.c
)

set_target_properties(serial PROPERTIES COMPILE_FLAGS -fPIC)


