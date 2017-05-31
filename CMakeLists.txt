cmake_minimum_required (VERSION 3.1)
set(PROJECT "dqueue")
project (${PROJECT} VERSION 0.0.1)

IF(NOT CMAKE_BUILD_TYPE)
        message(STATUS "CMAKE_BUILD_TYPE is not set, set to default = RELWITHDEBINFO")
        SET(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
ENDIF()

MESSAGE( STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE} )

option(DQUEUE_ENABLE_TESTS "Enable testing of the dqueue." ON)
option(DQUEUE_ENABLE_BENCHMARKS "Enable build benchmarks." ON)
option(DQUEUE_ENABLE_DOUBLECHECKS "More checks in code." OFF)
option(DQUEUE_ASAN_UBSAN "Enable address & undefined behavior sanitizer for binary." OFF)
option(DQUEUE_MSAN "Enable memory sanitizer for binary." OFF)
option(DQUEUE_SYSTEM_JEMALLOC "Use jemalloc installed in the system." ON)

if((DQUEUE_ASAN_UBSAN AND DQUEUE_MSAN))
  message(FATAL_ERROR "Sanitizers cannot be enabled simultaneously.")
endif()

MESSAGE(STATUS "Options: ")
MESSAGE(STATUS "DQUEUE_ENABLE_TESTS - " ${DQUEUE_ENABLE_TESTS})
MESSAGE(STATUS "DQUEUE_ENABLE_BENCHMARKS - " ${DQUEUE_ENABLE_BENCHMARKS})
MESSAGE(STATUS "DQUEUE_ENABLE_DOUBLECHECKS - " ${DQUEUE_ENABLE_DOUBLECHECKS})
MESSAGE(STATUS "DQUEUE_MSAN - " ${DQUEUE_MSAN})
MESSAGE(STATUS "DQUEUE_ASAN_UBSAN - " ${DQUEUE_ASAN_UBSAN})
MESSAGE(STATUS "DQUEUE_SYSTEM_JEMALLOC - " ${DQUEUE_SYSTEM_JEMALLOC})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddCXXCompilerFlag)
include(CompillerSettings)
include(CompillerSettings)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

set(BOOST_DEP system)

IF(DQUEUE_ENABLE_DOUBLECHECKS)
 add_definitions(-DDOUBLE_CHECKS)
ENDIF()

IF(WIN32 AND MSVC)
  find_package(Boost 1.54.0 COMPONENTS ${BOOST_DEP} REQUIRED)
else()
  set(BOOST_DEP ${BOOST_DEP} program_options)
  find_package(Boost  REQUIRED ${BOOST_DEP})
ENDIF()


########## BOOST

IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
  SET(DQUEUE_LIB_PATH ${Boost_LIBRARY_DIRS}
              CACHE STRING "DQUEUE libs path.")
else()
  MESSAGE(FATAL_ERROR "boost not found")
  EXIT()
ENDIF()


##### THREADS
find_package (Threads)

##### MISC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

UNSET(JEMALLOC_LIBS)

if(DQUEUE_SYSTEM_JEMALLOC)
   include(FindJeMalloc)
   if(JEMALLOC_FOUND)
      MESSAGE(STATUS "jemalloc-dev - found")
      SET(JEMALLOC_LIBS ${JEMALLOC_LIBRARIES})
   endif(JEMALLOC_FOUND)
else(DQUEUE_SYSTEM_JEMALLOC)
   MESSAGE(STATUS "LOCAL JEMALLOC")
   set(BuildTests OFF)
   add_subdirectory(extern/jemalloc-cmake)

   include_directories(extern/jemalloc-cmake/include/jemalloc)
   if(MSVC)
      include_directories(extern/jemalloc-cmake/include/msvc_compat)
   endif()
   SET(JEMALLOC_LIBS jemallocso)

   set_target_properties(jemalloc_jet PROPERTIES FOLDER Extern)
   set_target_properties(jemalloc_util_int_test PROPERTIES FOLDER Extern)
   set_target_properties(jemallocso PROPERTIES FOLDER Extern)
   set_target_properties(jemalloc PROPERTIES FOLDER Extern)
endif(DQUEUE_SYSTEM_JEMALLOC)

if(DQUEUE_ENABLE_TESTS)
   cmake_policy(SET CMP0048 OLD)
   include_directories(extern/Catch/include)
endif(DQUEUE_ENABLE_TESTS)

if(DQUEUE_ENABLE_BENCHMARKS)
   set(BENCHMARK_ENABLE_TESTING OFF)
   add_subdirectory(extern/benchmark)
   include_directories(extern/benchmark/include)
   set_target_properties(benchmark PROPERTIES FOLDER Extern)
endif(DQUEUE_ENABLE_BENCHMARKS)

SET(DQUEUE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "dqueue include dir.")

add_subdirectory (libdqueue)

if (DQUEUE_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(unittests)
endif()

if(DQUEUE_ENABLE_BENCHMARKS)
  add_subdirectory (microbenchmarks)
endif()
