cmake_minimum_required(VERSION 3.5)

project(Calculator CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
add_subdirectory(source bin)
