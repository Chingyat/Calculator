cmake_minimum_required(VERSION 3.8)

project(Interpreter CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(source bin)
