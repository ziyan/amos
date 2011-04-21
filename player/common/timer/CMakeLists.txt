add_library (timer STATIC
	timer.h
	timer.cc
)

set_target_properties(timer PROPERTIES COMPILE_FLAGS -fPIC)

