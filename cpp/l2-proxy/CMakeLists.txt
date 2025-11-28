cmake_minimum_required(VERSION 3.10)
project(l2-proxy)

# Find required packages
find_package(cpprestsdk REQUIRED)
find_package(hiredis REQUIRED)

# Add executable
add_executable(l2-proxy main.cpp)

# Link libraries
target_link_libraries(l2-proxy PRIVATE cpprestsdk::cpprest hiredis::hiredis)
