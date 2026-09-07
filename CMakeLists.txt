cmake_minimum_required(VERSION 3.16)
project(godot_terminal LANGUAGES CXX)

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

add_executable(godot_terminal
        src/Linux/main.cpp
        src/Linux/main.h
        src/Linux/register_types.cpp
        src/Linux/register_types.h
        src/Windows/main.cpp
        src/Windows/main.h
        src/Windows/register_types.cpp
        src/Windows/register_types.h
)

target_include_directories(godot_terminal PRIVATE
        ${CMAKE_SOURCE_DIR}/godot-cpp/gen/include
        ${CMAKE_SOURCE_DIR}/godot-cpp/include
)
