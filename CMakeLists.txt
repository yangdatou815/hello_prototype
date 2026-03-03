cmake_minimum_required(VERSION 3.10)
project(HelloWorldPrototype CXX)

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

# Add source code subdirectory
add_subdirectory(src)

# Add test subdirectories
add_subdirectory(test/ut)
# SCT is Python-based and run via tox, so no CMake integration needed here.
