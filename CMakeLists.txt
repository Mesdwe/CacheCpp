cmake_minimum_required(VERSION 3.10)

project(CachingPlayground)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")

add_executable(CacheTest ${SOURCES})
