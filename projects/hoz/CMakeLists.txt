cmake_minimum_required(VERSION 3.20)
project(hoz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)  # Allow fallback to C++20
set(CMAKE_CXX_EXTENSIONS OFF)

# Try C++23 first, fallback to C++20 if not available
include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <version>
#if __cplusplus < 202002L
#error \"C++20 or higher required\"
#endif
int main() { return 0; }
" CXX20_SUPPORTED)

check_cxx_source_compiles("
#include <version>
#if __cplusplus < 202302L
#error \"C++23 or higher required\"
#endif
int main() { return 0; }
" CXX23_SUPPORTED)

if(NOT CXX23_SUPPORTED)
    message(STATUS "C++23 not supported, falling back to C++20")
    set(CMAKE_CXX_STANDARD 20)
    add_compile_definitions(HOZ_USE_CXX20)
else()
    message(STATUS "C++23 supported")
endif()

add_subdirectory(deps/glaze)

# Get git commit hash for version info
find_package(Git)
if(Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --dirty --abbrev=7
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT GIT_COMMIT_HASH)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
  endif()
else()
  set(GIT_COMMIT_HASH "unknown")
endif()

add_executable(hoz
    src/main.cpp
    src/resolver.cpp
    src/commands.cpp)
target_link_libraries(hoz PRIVATE glaze::glaze)
target_compile_definitions(hoz PRIVATE HOZ_VERSION="${PROJECT_VERSION}")
target_compile_definitions(hoz PRIVATE HOZ_COMMIT_HASH="${GIT_COMMIT_HASH}")

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(hoz PRIVATE -Wall -Wextra -Wpedantic)
endif()
