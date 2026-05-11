find_package(doctest CONFIG REQUIRED)

add_executable(HiveWE_tests
	test_main.cpp
	modification_tables_test.cpp
	binary_writer_test.cpp
)

target_link_libraries(HiveWE_tests PRIVATE
	HiveWE_lib
	doctest::doctest
)

include(doctest)
doctest_discover_tests(HiveWE_tests)
