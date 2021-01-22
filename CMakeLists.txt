cmake_minimum_required(VERSION 3.17)

option(BUILD_STATIC "Build static lib" OFF)
option(BUILD_SHARED "Build shared lib" ON)

if (BUILD_STATIC)
    set(BUILD_SHARED OFF)
endif()

project(XmlProcessing LANGUAGES CXX)

set(LIB_NAME "xml")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    inc/
    )
file (GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/inc/*.hpp
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    )
if(BUILD_SHARED)
    add_library(${LIB_NAME} SHARED ${SRC_FILES})
endif()
if(BUILD_STATIC)
    add_library(${LIB_NAME} STATIC ${SRC_FILES})
endif()
set_target_properties(${LIB_NAME} PROPERTIES VERSION 1.1)
