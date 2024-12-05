macro(add_xin_database_test target source)
    add_executable(${target} ${source})

    target_link_libraries(${target}
        PRIVATE
            Catch2::Catch2WithMain
            xin::database
        )
    catch_discover_tests(${target})
endmacro()

add_xin_database_test(test-coding test_coding.cpp)
add_xin_database_test(test-write-batch test_write_batch.cpp)
