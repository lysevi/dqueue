INCLUDE_DIRECTORIES(${DQUEUE_INCLUDE_DIR})

MESSAGE(STATUS "-- echo_queue")
add_executable(benchmark_echo_queue echo_queue.cpp)
TARGET_LINK_LIBRARIES(benchmark_echo_queue ${DQUEUE_LIBS})
set_target_properties(benchmark_echo_queue PROPERTIES FOLDER Benchmarks)

add_test(benchmark_echo_queue ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmark_echo_queue -Q 1 -S 1 --clients=1 -D)