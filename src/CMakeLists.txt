# Add a library
add_library(hello_lib
    Peer.cpp
    Session.cpp
    Server.cpp
    Client.cpp
    Crypto.cpp
    Database.cpp
)

target_include_directories(hello_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenSSL REQUIRED)
target_link_libraries(hello_lib PUBLIC asio_lib OpenSSL::SSL OpenSSL::Crypto sqlite3_lib dl)

# Add an executable
add_executable(hello_app main.cpp)
target_link_libraries(hello_app hello_lib)
