# Tests for smd_parser. Each test_*.cpp is a standalone binary that
# loads .smd files from the working directory. CMake sets WORKING_DIRECTORY
# for each ctest invocation to the modes dir.

set(SMD_TESTS
    test_array_index_mangling
    test_predefined_names
    test_struct_var
    test_lifecycle
    test_brace_ternary
    test_all_smd_files
    test_fixture_discovery
    test_render_output
    test_stress
    test_edge_cases
    test_many_formats
    test_history
    test_strict_directives
    test_memory_growth
    test_color_swap
    test_reset_refresh
    test_error_wrap
	test_history_average
)

set(FIXTURES_DIR "${CMAKE_SOURCE_DIR}/modes")

foreach(t ${SMD_TESTS})
    add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE smd_parser)
    # Tests share helpers in test_support.hpp under tests/.
    target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${t}
        COMMAND ${t}
        WORKING_DIRECTORY ${FIXTURES_DIR}
    )
    set_tests_properties(${t} PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:abort_on_error=0:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1;UBSAN_OPTIONS=halt_on_error=0:print_stacktrace=1"
    )
endforeach()

# Per-frame heap-growth check uses mallinfo2 to spot leaks that AddressSanitizer
# can't see (the leaks we fixed were eventually freed at process exit, so
# ASan flagged nothing -- but the overlay would still OOM after ~15s on
# Switch). Sanitizers inflate allocator metadata and would skew uordblks
# readings, so this test is excluded from sanitizer-enabled builds.
if(SMD_SANITIZE)
    set_tests_properties(test_memory_growth PROPERTIES DISABLED TRUE)
endif()
