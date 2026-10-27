# 1. Minimum version and Project Name
cmake_minimum_required(VERSION 3.18)
project(TIVIDY_Project LANGUAGES CXX)

# 2. Set C++11 Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. Tell CMake where headers are 
include_directories(include)

# 4. Automatically find all .cpp 
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# 5. Create your main application executable
add_executable(TIVIDY_App ${ALL_SOURCES})

# 6. Create your Test executable, written with GoogleTest (vendored in third_party/googletest)
#    Version 1.12.1 is the last GoogleTest release that supports C++11.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(GOOGLETEST_VERSION 1.12.1)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

# Remove the app's main.cpp from the list of sources used for tests
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Add the test files AND the core/pattern source files
add_executable(TIVIDY_Tests ${TEST_SOURCES} ${ALL_SOURCES})
target_link_libraries(TIVIDY_Tests PRIVATE gtest_main Threads::Threads)

# 7. CTest: `ctest` runs every GoogleTest case on its own
enable_testing()
include(GoogleTest)
gtest_discover_tests(TIVIDY_Tests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)
