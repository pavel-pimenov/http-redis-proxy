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
add_executable(l2-worker
main.cpp
civetweb/CivetServer.cpp
civetweb/civetweb.c
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
)

# Include directories
target_include_directories(l2-worker PRIVATE ${HIREDIS_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIRS})

include_directories(prometheus-cpp/core/include)
include_directories(prometheus-cpp/util/include)
include_directories(prometheus-cpp/pull/include)
include_directories(civetweb)

# Link libraries
target_link_libraries(l2-worker PRIVATE CURL::libcurl ${HIREDIS_LIBRARY} ${JSONCPP_LIBRARIES})
