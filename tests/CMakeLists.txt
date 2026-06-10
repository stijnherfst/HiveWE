find_package(doctest CONFIG REQUIRED)

add_executable(HiveWE_tests
	test_main.cpp
	modification_tables_test.cpp
	binary_writer_test.cpp
	mdl_reader_test.cpp
	validator_test.cpp
)

target_link_libraries(HiveWE_tests PRIVATE
	HiveWE_lib
	doctest::doctest
)

target_compile_definitions(HiveWE_tests PRIVATE
	MDL_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mdl"
)

include(doctest)
doctest_discover_tests(HiveWE_tests)
