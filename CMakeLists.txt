# 1. Minimum version and Project Name
#    (3.14 is needed for FetchContent_MakeAvailable)
cmake_minimum_required(VERSION 3.14)
project(TIVIDY_Project LANGUAGES CXX)

# 2. Fetch FTXUI (terminal UI library).
#    Done BEFORE the C++11 setting below so FTXUI is not forced to C++11.
include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG        v5.0.0)
FetchContent_MakeAvailable(ftxui)

# 3. Set C++11 Standard (engine and tests stay C++11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. Tell CMake where headers are
include_directories(include)

# 5. Automatically find all .cpp
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

# 6. Main application executable (now the FTXUI TUI)
add_executable(TIVIDY_App ${ALL_SOURCES})
target_link_libraries(TIVIDY_App PRIVATE
    ftxui::screen ftxui::dom ftxui::component)
# FTXUI needs C++17, so only the app target is raised to 17.
set_target_properties(TIVIDY_App PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# 7. Test executable
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

# Remove the app's main.cpp from the list of sources used for tests
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Add the test files AND the core/pattern source files
add_executable(TIVIDY_Tests ${TEST_SOURCES} ${ALL_SOURCES})

# 8. Optional: the old text REPL as its own executable.
#    Keep cli_main.cpp OUTSIDE src/ (e.g. legacy/) or it will be globbed
#    into TIVIDY_App and clash with the TUI main.
if(EXISTS "${CMAKE_SOURCE_DIR}/legacy/cli_main.cpp")
    add_executable(TIVIDY_CLI legacy/cli_main.cpp ${ALL_SOURCES})
endif()
