cmake_minimum_required(VERSION 3.15)
project(redis_filtered_sort)
set(LANGUAGE CXX)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_VERBOSE_MAKEFILE on )
set(Boost_USE_STATIC_LIBS  Off)

find_package(Boost REQUIRED COMPONENTS thread)

MESSAGE(Boost ${Boost_LIBRARY_DIR_RELEASE})

include_directories(src/)
include_directories(src/command)
include_directories(src/redis)
include_directories(src/util)
include_directories(src/util/filter)

FILE(GLOB_RECURSE SOURCE_FILES src/*/*.cpp)
set (CMAKE_CXX_FLAGS "-g -Wall")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} pthread Boost::thread)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} PUBLIC src/filter-module)
