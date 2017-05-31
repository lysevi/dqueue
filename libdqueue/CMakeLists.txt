include(GenerateExportHeader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
INCLUDE_DIRECTORIES(${DQUEUE_INCLUDE_DIR})

FILE(GLOB GLOB_HDRS "*.h")
FILE(GLOB UTILS_HDRS "utils/*.h")
FILE(GLOB UTILS_ASYNC_HDRS "utils/async/*.h")

FILE(GLOB GLOB_SRC "*.cpp")
FILE(GLOB UTILS_SRC "utils/*.cpp")
FILE(GLOB UTILS_ASYNC_SRC "utils/async/*.cpp")

source_group(utils FILES ${UTILS_HDRS} ${UTILS_SRC})
source_group(utils\\async FILES ${UTILS_ASYNC_HDRS} ${UTILS_ASYNC_SRC})
source_group(dqueue FILES ${GLOB_HDRS} ${GLOB_SRC})

SET(SRC  
     ${GLOB_HDRS} ${GLOB_SRC} 
	 ${UTILS_HDRS}  ${UTILS_SRC} 
	 ${UTILS_ASYNC_HDRS}  ${UTILS_ASYNC_SRC})

SET(DQUEUE_LIBS dqueue ${Boost_LIBRARIES} ${JEMALLOC_LIBS} CACHE STRING "dqueue libs.")

add_library(dqueue SHARED ${SRC})
TARGET_LINK_LIBRARIES(dqueue ${Boost_LIBRARIES} ${JEMALLOC_LIBS} ${CMAKE_THREAD_LIBS_INIT})

IF(WIN32)
 target_link_libraries(dqueue wsock32 ws2_32)
endif()

GENERATE_EXPORT_HEADER(dqueue
    BASE_NAME dqueue
    EXPORT_MACRO_NAME EXPORT
    EXPORT_FILE_NAME exports.h
    STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC)
