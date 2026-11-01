# 1. Minimum version and Project Name
cmake_minimum_required(VERSION 3.18)
project(TIVIDY_Project LANGUAGES C CXX)

# 2. Set C++11 Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 3. Tell CMake where headers are 
include_directories(include)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "src/core/*.cpp" "src/patterns/*.cpp")
add_library(tividy_engine STATIC ${ENGINE_SOURCES})

file(GLOB_RECURSE SERVICE_SOURCES CONFIGURE_DEPENDS "src/server/*.cpp")
add_library(tividy_service STATIC ${SERVICE_SOURCES})
target_link_libraries(tividy_service PUBLIC tividy_engine)

add_library(civetweb STATIC third_party/civetweb/civetweb.c)
target_include_directories(civetweb PUBLIC third_party/civetweb)
target_compile_definitions(civetweb PRIVATE NO_SSL NO_CGI NO_CACHING USE_WEBSOCKET)
target_link_libraries(civetweb PUBLIC Threads::Threads)
if(WIN32)
    target_link_libraries(civetweb PUBLIC ws2_32)
else()
    target_link_libraries(civetweb PUBLIC ${CMAKE_DL_LIBS})
endif()

add_executable(TIVIDY_App src/main.cpp)
target_link_libraries(TIVIDY_App PRIVATE tividy_service)
target_compile_definitions(TIVIDY_App PRIVATE TIVIDY_SCENARIOS="${CMAKE_SOURCE_DIR}/scenarios")

file(GLOB NET_SOURCES CONFIGURE_DEPENDS "src/net/*.cpp")
add_executable(TIVIDY_Server ${NET_SOURCES})
target_link_libraries(TIVIDY_Server PRIVATE tividy_service civetweb Threads::Threads)
target_compile_definitions(TIVIDY_Server PRIVATE TIVIDY_WEB_ROOT="${CMAKE_SOURCE_DIR}/web"
                                                TIVIDY_SCENARIOS="${CMAKE_SOURCE_DIR}/scenarios")

set(GOOGLETEST_VERSION 1.12.1)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
add_executable(TIVIDY_Tests ${TEST_SOURCES})
target_link_libraries(TIVIDY_Tests PRIVATE tividy_service gtest_main Threads::Threads)

enable_testing()
include(GoogleTest)
gtest_discover_tests(TIVIDY_Tests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)
