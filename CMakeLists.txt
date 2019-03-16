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
	"experimental/example_json.cpp"

	"experimental/y_json.h"
	"experimental/y_json.cpp"
)

#-----------------------------------------------------------------------

add_executable ("example_profiler"
	"experimental/example_profiler.cpp"

	"experimental/y_profiler.h"
	"experimental/y_profiler.cpp"
)

#-----------------------------------------------------------------------

add_executable ("example_ecs"
	"experimental/example_ecs.cpp"

	"experimental/y_ecs.hpp"
	"experimental/y_ecs.cpp"
)

#-----------------------------------------------------------------------

add_executable ("tests"
	"tests/catch.hpp"
	"tests/tests_main.cpp"

	"ylib/y_fiber.h"
	"ylib/y_fiber.cpp"
	
	"ylib/y_lockfree.hpp"
	
	"experimental/y_json.h"
	"experimental/y_json.cpp"
    "tests/tests_json.cpp"
	
	"experimental/y_ring_allocator.hpp"
	"experimental/y_ring_allocator.cpp"
	"tests/tests_ring_allocator.cpp"

	"experimental/y_array.hpp"
	"tests/tests_array.cpp"

	"experimental/y_map.hpp"
	"tests/tests_map.cpp"
	
	"experimental/y_stats.hpp"
	"tests/tests_stats.cpp"
	
	"experimental/y_profiler.h"
	"experimental/y_profiler.cpp"
	"tests/tests_profiler.cpp"
	
	"ylib/y_basics.hpp"
	"tests/tests_basics.cpp"
	
	"ylib/y_format.hpp"
	"tests/tests_format.cpp"
	
	"experimental/y_string.hpp"
	"tests/tests_string.cpp"
	
	"experimental/y_bignum.hpp"
	"experimental/y_bignum.cpp"
	"tests/tests_bignum.cpp"
	
	"experimental/y_socket.hpp"
	"experimental/y_socket.cpp"
	"tests/tests_socket.cpp"
	
	"experimental/y_string_conversion.hpp"
	"tests/tests_string_conversion.cpp"
)

#-----------------------------------------------------------------------
#-----------------------------------------------------------------------
