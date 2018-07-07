cmake_minimum_required (VERSION 3.0)

project ("yLib_${CMAKE_GENERATOR}" CXX)
#http://www.cmake.org/Wiki/CMake_Useful_Variables

#option(BUILD_TESTS "Build tests" ON)
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE INTERNAL "" FORCE)
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE INTERNAL "" FORCE)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) 
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) 

include_directories ("ylib")

if (MSVC)
	add_definitions (/WX)		# Warnings as errors
	add_definitions (/W4)		# Warning level 4
	add_definitions (/wd4100)	# Disable warning about "unreferenced formal parameter"
	add_definitions (/EHsc)
	add_definitions (-D_CRT_SECURE_NO_WARNINGS)
	add_definitions (/std:c++latest)
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
	add_definitions (-Wall)
	#add_definitions (-Wno-invalid-offsetof)
else ()	# hopefully, it's Clang!
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
	add_definitions (-Wall)
	#add_definitions (-Wno-invalid-offsetof)
endif ()

#-----------------------------------------------------------------------
#-----------------------------------------------------------------------
#-----------------------------------------------------------------------

add_executable ("example_fiber"
	"examples/example_fiber.cpp"

	"ylib/y_fiber.h"
	"ylib/y_fiber.cpp"
)

#-----------------------------------------------------------------------

add_executable ("example_lockfreequeue"
	"examples/example_lockfreequeue.cpp"

	"ylib/y_lockfree.hpp"
)

#-----------------------------------------------------------------------

add_executable ("example_json"
	"examples/example_json.cpp"

	"ylib/y_json.h"
	"ylib/y_json.cpp"
)

#-----------------------------------------------------------------------

add_executable ("tests"
	"tests/catch.hpp"
	"tests/tests_main.cpp"

	"ylib/y_fiber.h"
	"ylib/y_fiber.cpp"
	
	"ylib/y_lockfree.hpp"
	
	"ylib/y_json.h"
	"ylib/y_json.cpp"
	
	"ylib/y_ring_allocator.hpp"
	"ylib/y_ring_allocator.cpp"
	"tests/tests_ring_allocator.cpp"

	"ylib/y_array.hpp"
	"tests/tests_array.cpp"

	"ylib/y_map.hpp"
	"tests/tests_map.cpp"
)

#-----------------------------------------------------------------------
#-----------------------------------------------------------------------
