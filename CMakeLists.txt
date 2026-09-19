# Minimum CMake version required, and the project name/language.
cmake_minimum_required(VERSION 3.20)
project(warehouse_routing_capstone LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use C++20 throughout the project.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Brings in the ability to download dependencies at configure time.
include(FetchContent)

# --- GoogleTest ---
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.18.0
)
FetchContent_MakeAvailable(googletest)

# --- nlohmann/json ---
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Your project's own library (Grid, Graph, later A*/CBS) lives in src/.
add_subdirectory(src)

# Turns on CTest integration for this whole project - required for
# `ctest` to find and report anything.
enable_testing()

# Your test executable and its registered tests live in tests/.
add_subdirectory(tests)