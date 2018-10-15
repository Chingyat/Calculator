add_executable(calc main.cpp)
target_link_libraries(calc Threads::Threads)
target_compile_features(calc PUBLIC cxx_std_17)

target_link_libraries(calc "readline")
