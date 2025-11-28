cmake_minimum_required(VERSION 3.10)
project(l2-server)

# Find required packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Add executable
add_executable(l2-server main.cpp)

# Link libraries and include directories
target_link_libraries(l2-server PRIVATE ${MICROHTTPD_LIBRARIES} ${JSONCPP_LIBRARIES})
target_include_directories(l2-server PRIVATE ${MICROHTTPD_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS})
