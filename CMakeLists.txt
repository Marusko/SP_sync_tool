cmake_minimum_required(VERSION 3.18)
project(SP_sync_tool)

set(CMAKE_CXX_STANDARD 17)

add_executable(SP_sync_tool main.cpp
        Client.h
        Server.h
        Client.cpp
        messages.h)
