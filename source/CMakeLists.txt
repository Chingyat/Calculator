add_executable(calc main.cpp calculator.cpp)

target_link_libraries(calc Threads::Threads)
target_compile_features(calc PRIVATE cxx_std_17)

target_link_libraries(calc "readline")

