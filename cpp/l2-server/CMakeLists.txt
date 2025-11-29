cmake_minimum_required(VERSION 3.10)
project(l2-server)

# Add executable
add_executable(l2-server main.cpp)

# Link with threading library
target_link_libraries(l2-server PRIVATE pthread)
