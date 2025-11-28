cmake_minimum_required(VERSION 3.10)
project(l2-worker)

# Find required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Find hiredis manually
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

# Add executable
add_executable(l2-worker main.cpp)

# Include directories
target_include_directories(l2-worker PRIVATE ${HIREDIS_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIRS})

# Link libraries
target_link_libraries(l2-worker PRIVATE CURL::libcurl ${HIREDIS_LIBRARY} ${JSONCPP_LIBRARIES})
