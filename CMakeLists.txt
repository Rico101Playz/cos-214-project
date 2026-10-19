# 1. Minimum version and Project Name
cmake_minimum_required(VERSION 3.10)
project(TIVIDY_Project LANGUAGES CXX)

# 2. Set C++11 Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. Tell CMake where headers are 
include_directories(include)

# 4. Automatically find all .cpp 
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

# 5. Create your main application executable
add_executable(TIVIDY_App ${ALL_SOURCES})

# 6. Create your Test executable
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

# Remove the app's main.cpp from the list of sources used for tests
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Add the test files AND the core/pattern source files
add_executable(TIVIDY_Tests ${TEST_SOURCES} ${ALL_SOURCES})