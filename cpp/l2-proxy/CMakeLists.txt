cmake_minimum_required(VERSION 3.10)
project(l2-proxy)

# Find required packages
find_package(OpenSSL REQUIRED)

# Find hiredis manually
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h REQUIRED)
find_library(HIREDIS_LIBRARY hiredis REQUIRED)

# Add CivetWeb as external project
# include(ExternalProject)
# ExternalProject_Add(
#    civetweb
#    GIT_REPOSITORY https://github.com/civetweb/civetweb.git
#    GIT_TAG v1.16
#    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCIVETWEB_ENABLE_SSL=ON -DCIVETWEB_BUILD_TESTING=OFF -DCIVETWEB_ENABLE_CXX=ON
#    INSTALL_COMMAND ""
#)

 # TODO
 #sqlite/sqlite3x_command.cpp
 #sqlite/sqlite3x_connection.cpp
 #sqlite/sqlite3x_reader.cpp
 #sqlite/sqlite3x_transaction.cpp
 #sqlite/sqlite3.c


# Add executable
add_executable(l2-proxy 
main.cpp
jsoncpp/jsoncpp.cpp
 prometheus-cpp/pull/src/basic_auth.cc
 prometheus-cpp/pull/src/endpoint.cc
 prometheus-cpp/pull/src/exposer.cc
 prometheus-cpp/pull/src/handler.cc
 prometheus-cpp/pull/src/metrics_collector.cc
 prometheus-cpp/core/src/check_names.cc
 prometheus-cpp/core/src/counter.cc
 prometheus-cpp/core/src/family.cc
 prometheus-cpp/core/src/gauge.cc
 prometheus-cpp/core/src/histogram.cc
 prometheus-cpp/core/src/info.cc
 prometheus-cpp/core/src/registry.cc
 prometheus-cpp/core/src/serializer.cc
 prometheus-cpp/core/src/summary.cc
 prometheus-cpp/core/src/text_serializer.cc
 prometheus-cpp/core/src/detail/builder.cc
 prometheus-cpp/core/src/detail/ckms_quantiles.cc
 prometheus-cpp/core/src/detail/time_window_quantiles.cc
 prometheus-cpp/core/src/detail/utils.cc
 civetweb/CivetServer.cpp
 civetweb/civetweb.c
)

# Include directories
target_include_directories(l2-proxy PRIVATE ${HIREDIS_INCLUDE_DIR})
target_include_directories(l2-proxy PRIVATE ${SOURCE_DIR}/include)

# Link libraries
target_link_libraries(l2-proxy PRIVATE ${HIREDIS_LIBRARY} OpenSSL::SSL OpenSSL::Crypto)

include_directories(prometheus-cpp/core/include)
include_directories(prometheus-cpp/include)
include_directories(prometheus-cpp/util/include)
include_directories(prometheus-cpp/pull/include)
include_directories(civetweb)
include_directories(jsoncpp)
include_directories(sqlite)

target_link_libraries(l2-proxy PRIVATE ${HIREDIS_LIBRARY})
