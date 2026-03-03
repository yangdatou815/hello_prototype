# Add a library
add_library(hello_lib
    Peer.cpp
    Session.cpp
    Server.cpp
    Client.cpp
)

target_include_directories(hello_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hello_lib PUBLIC asio_lib)

# Add an executable
add_executable(hello_app main.cpp)

# Link the library to the executable
target_link_libraries(hello_app hello_lib)
