cmake_minimum_required(VERSION 3.10)
project(HelloWorldPrototype C CXX)

include(FetchContent)

# Fetch standalone Asio (header-only, no Boost required)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-28-0
)
FetchContent_MakeAvailable(asio)
add_library(asio_lib INTERFACE)
target_include_directories(asio_lib INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_lib INTERFACE ASIO_STANDALONE)
find_package(Threads REQUIRED)
target_link_libraries(asio_lib INTERFACE Threads::Threads)

# Fetch SQLite3 amalgamation (single C file, no system install needed)
FetchContent_Declare(
  sqlite3
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
)
FetchContent_MakeAvailable(sqlite3)
add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
set_target_properties(sqlite3_lib PROPERTIES LINKER_LANGUAGE C)
target_compile_options(sqlite3_lib PRIVATE -w)   # suppress warnings in 3rd party code

# Add public include directory
include_directories(include)

# Enable CTest at the top level so sub-directory tests are registered
enable_testing()

# Add source code subdirectory
add_subdirectory(src)

# Add test subdirectories
add_subdirectory(test/ut)
# SCT is Python-based and run via tox, so no CMake integration needed here.
