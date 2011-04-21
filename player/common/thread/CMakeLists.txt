add_library (thread STATIC
	thread.h
	thread.cc
	mutex.h
	mutex.cc
)

set_target_properties(thread PROPERTIES COMPILE_FLAGS -fPIC)

target_link_libraries(thread -lpthread)



