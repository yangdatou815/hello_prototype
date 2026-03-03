cmake_minimum_required(VERSION 3.10)
project(HelloWorldUT)

# Get GTest from git
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

# Add test executable
add_executable(ut_greeter test_greeter.cpp)
target_link_libraries(ut_greeter PRIVATE hello_lib GTest::gtest_main)
target_include_directories(ut_greeter PRIVATE ${googletest_SOURCE_DIR}/googletest/include)
