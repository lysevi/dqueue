INCLUDE_DIRECTORIES(${DQUEUE_INCLUDE_DIR})

MESSAGE(STATUS "-- connection")
add_executable(connection_test connection_test.cpp)
TARGET_LINK_LIBRARIES(connection_test ${DQUEUE_LIBS})
set_target_properties(connection_test PROPERTIES FOLDER Benchmarks)

#add_test(connection_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/connection_test)