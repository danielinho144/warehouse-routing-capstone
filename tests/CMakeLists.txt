#creates test executable from the test files
add_executable(warehouse_routing_tests
test_grid.cpp
test_graph.cpp
test_map_loader.cpp
test_agent.cpp
test_order.cpp
test_scenario_loader.cpp
test_astar.cpp
test_independent_planner.cpp
test_order_assignment.cpp
test_simulation.cpp
test_conflict_detection.cpp
test_constrained_astar.cpp
test_inventory.cpp
test_pickup_task.cpp
test_simulation_fulfillment.cpp
test_cbs.cpp
test_destination_access.cpp
)

#link in warehouse_routing_tests and gtest_main
#so tests can use the core library and GoogleTest
target_link_libraries(warehouse_routing_tests PRIVATE
warehouse_routing_core
gtest_main
)

target_compile_definitions(warehouse_routing_tests PRIVATE
  MAP_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data/warehouses/test_cases"
  SCENARIO_DATA_DIR="${CMAKE_SOURCE_DIR}/data/scenarios"
)

#load CMake module that provides gtest_discover_tests
include(GoogleTest)

#find every TEST()
#each with CTest so 'ctest' can run individually
gtest_discover_tests(warehouse_routing_tests)
