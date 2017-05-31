INCLUDE_DIRECTORIES(${DQUEUE_INCLUDE_DIR})

MESSAGE(STATUS "Unittests")

FILE(GLOB HDRS "*.h")
FILE(GLOB SRC "*.cpp")

add_executable(unittests ${HDRS} ${SRC})
TARGET_LINK_LIBRARIES(unittests ${DQUEUE_LIBS})
add_test(unittests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unittests) 
set_target_properties(unittests PROPERTIES FOLDER Tests)
