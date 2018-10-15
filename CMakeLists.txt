cmake_minimum_required(VERSION 3.5)

project(Calculator CXX)

find_package(Threads REQUIRED)
add_subdirectory(source bin)
