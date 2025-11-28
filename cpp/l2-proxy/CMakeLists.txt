cmake_minimum_required(VERSION 3.10)
project(l2-proxy)

# Find required packages
find_package(cpprestsdk REQUIRED)
find_package(OpenSSL REQUIRED)

# Find hiredis manually
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

# Add executable
add_executable(l2-proxy main.cpp)

# Include directories
target_include_directories(l2-proxy PRIVATE ${HIREDIS_INCLUDE_DIR})

# Link libraries
target_link_libraries(l2-proxy PRIVATE cpprestsdk::cpprest ${HIREDIS_LIBRARY} OpenSSL::SSL OpenSSL::Crypto)
