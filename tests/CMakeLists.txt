cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# Explicitly list the test source code and headers. The Catch header-only unit
# test framework is stored in with the test source.

# TODO(jhpark): real test !!!
set(YCSBC_TEST_SRC
    test_cmake_demo.cpp
)
set(YCSBC_TEST_HEADER
    catch.hpp
)

#PREPEND(YCSBC_TEST_SRC)

# Make an executable target that depends on the test source code we specified
# above.
add_executable(TestYCSBC ${YCSBC_TEST_SRC} ${YCSBC_TEST_HEADER})

# Enable testing via CTest
enable_testing()
# Add our test as runnable via CTest
add_test(NAME TestYCSBC  COMMAND TestYCSBC)

# Link our unit tests against the library we compiled
target_link_libraries(TestYCSBC YCSBC)
