# Basic CMakeLists.txt for JetBrains IDEA users.
cmake_minimum_required(VERSION 4.0)
project(RebelSQLiteLibrary)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.c**)
file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/*.h**)
include_directories(.)
include_directories(RebelSDK/include)
include_directories(RebelSDK/include/api)
include_directories(third-party)
add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
