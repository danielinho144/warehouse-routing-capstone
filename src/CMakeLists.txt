# Builds Grid, Graph, and (later) the A*/CBS solvers into one library
# that both the main project and the tests link against.
add_library(warehouse_routing_core
  grid.cpp
  graph.cpp
  map_loader.cpp
  agent.cpp
  order.cpp
  scenario_loader.cpp
  astar.cpp
  independent_planner.cpp
)

# Tells the compiler to treat include/ as a search root, so
# #include "warehouse_routing/grid.h" resolves correctly - from
# anywhere that links against this library.
target_include_directories(warehouse_routing_core PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# Links the nlohmann/json library (pulled in via FetchContent at
# the top level) so grid.cpp/graph.cpp can use it if/when needed.
target_link_libraries(warehouse_routing_core PUBLIC
  nlohmann_json::nlohmann_json
)