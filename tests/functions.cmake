function(an_add_tests)
    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to an_add_tests()")
    endif()

    set(options)

    set(oneValueArgs
            TARGET)

    set(multiValueArgs
            SOURCES
            DEPENDENCIES)

    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TEST_TARGET ${ARGS_TARGET}_test)
    add_executable(${TEST_TARGET} ${ARGS_SOURCES})
    target_include_directories(${TEST_TARGET} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${TEST_TARGET} PUBLIC GTest::GTest GTest::Main ${ARGS_DEPENDENCIES})
    add_test(${ARGS_TARGET} ${TEST_TARGET})
endfunction()