cmake_minimum_required(VERSION 3.18)
project(SP_sync_tool)

set(CMAKE_CXX_STANDARD 20)

add_executable(SP_sync_tool main.cpp
        Client.h
        Server.h
        Client.cpp
        messages.h
        Server.cpp)
target_link_libraries(SP_sync_tool pthread)