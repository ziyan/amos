cmake_minimum_required (VERSION 2.6 FATAL_ERROR)
project (amos)

set (CMAKE_BUILD_TYPE debug)
set (CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -Wall")
set (CMAKE_C_FLAGS_DEBUG "-g -ggdb -Wall")
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
set (EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin" CACHE PATH "EXECUTABLE_OUTPUT_PATH" FORCE)
set (LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib" CACHE PATH "LIBRARY_OUTPUT_PATH" FORCE)
mark_as_advanced (EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)


set (COMMON_DIR "${PROJECT_SOURCE_DIR}/common")

add_subdirectory (common)
add_subdirectory (plugins)
add_subdirectory (clients)
add_subdirectory (utils)

