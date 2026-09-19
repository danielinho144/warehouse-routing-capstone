#creates test executable, build from both test files
add_executable(warehouse_routing_tests
test_grid.cpp
test_graph.cpp
)

#link in warehouse_routing_tests and gtest_main
#so tests can use grid
target_link_libraries(warehouse_routing_tests PRIVATE
warehouse_routing_core
gtest_main
)

#load CMake module that provides gtest_discover_tests
include(GoogleTest)

#find every TEST()
#each with CTest so 'ctest' can run individually
gtest_discover_tests(warehouse_routing_tests)