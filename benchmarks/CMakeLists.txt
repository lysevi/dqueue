INCLUDE_DIRECTORIES(${DQUEUE_INCLUDE_DIR})

MESSAGE(STATUS "-- echo_queue")
add_executable(benchmark_echo_queue echo_queue.cpp)
TARGET_LINK_LIBRARIES(benchmark_echo_queue ${DQUEUE_LIBS})
set_target_properties(benchmark_echo_queue PROPERTIES FOLDER Benchmarks)

#add_test(benchmark_echo_queue ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmark_echo_queue -Q 1 -S 1 --clients=1 -D)

MESSAGE(STATUS "-- connection")
add_executable(connection_test connection_test.cpp)
TARGET_LINK_LIBRARIES(connection_test ${DQUEUE_LIBS})
set_target_properties(connection_test PROPERTIES FOLDER Benchmarks)

add_test(connection_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/connection_test)