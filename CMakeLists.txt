cmake_minimum_required(VERSION 3.16)
project(godot_terminal LANGUAGES CXX)

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

add_executable(godot_terminal
        src/Linux-New/main.cpp
        src/Linux-New/main.h
)

target_include_directories(godot_terminal PRIVATE
        ${CMAKE_SOURCE_DIR}/godot-cpp/gen/include
        ${CMAKE_SOURCE_DIR}/godot-cpp/include
)
