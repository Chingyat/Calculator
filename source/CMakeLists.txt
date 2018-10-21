add_executable(calc
               main.cpp
               interpreter.cpp
               astimpl.cpp
               parser.cpp
               demangle.cpp)

target_link_libraries(calc Threads::Threads)
target_compile_features(calc PUBLIC cxx_std_17)

target_link_libraries(calc "readline")

add_library(stdlib SHARED stdlib.cpp)
target_compile_features(stdlib PUBLIC cxx_std_17)

target_link_libraries(calc stdlib)
target_link_libraries(calc fmt)
