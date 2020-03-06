# Automatically discovered unit tests.
#
# Test discovery can be an issue when dealing with unit tests in C and C++.
# Automatically adding test file to the build and automatically registering
# tests simplifies the workflow considerably.
file(GLOB_RECURSE KNELL_UNIT_TEST_SRCS ./test-*.c)

foreach(TEST_EXE_NAME ${KNELL_UNIT_TEST_SRCS})
    get_filename_component(TEST_NAME ${TEST_EXE_NAME} NAME_WE)
    message("Adding unit test: ${TEST_NAME}")
    add_unit_test(NAME ${TEST_NAME} SOURCES ${TEST_EXE_NAME} LIBS knell-lib-test)
endforeach()