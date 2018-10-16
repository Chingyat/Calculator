add_executable(calc main.cpp calculator.cpp)

target_link_libraries(calc Threads::Threads)
target_compile_features(calc PRIVATE 
	cxx_defaulted_functions
	cxx_lambda_init_captures
	cxx_final
	cxx_range_for
	)

target_link_libraries(calc "readline")

