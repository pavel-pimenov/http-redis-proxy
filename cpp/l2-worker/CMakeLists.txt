cmake_minimum_required(VERSION 3.10)
project(l2-worker)

# Find required packages
find_package(CURL REQUIRED)
find_package(hiredis REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Add executable
add_executable(l2-worker main.cpp)

# Link libraries and include directories
target_link_libraries(l2-worker PRIVATE CURL::libcurl hiredis::hiredis ${JSONCPP_LIBRARIES})
target_include_directories(l2-worker PRIVATE ${JSONCPP_INCLUDE_DIRS})
