cmake_minimum_required(VERSION 3.10)
project(rendering_osg)

include(cmake/osg.cmake)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 直接指定要包含的目录
set(INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/include/common
	${CMAKE_CURRENT_SOURCE_DIR}/include/manager
	${CMAKE_CURRENT_SOURCE_DIR}/include/utils
    # 可以添加更多目录
    # "${CMAKE_CURRENT_SOURCE_DIR}/other/include"
)
message(STATUS "Include directories: ${INCLUDE_DIRS}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE HEADER_FILES ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.cc ${SOURCE_DIR}/*.c)
set(SOURCES ${HEADER_FILES} ${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

source_group(TREE ${SOURCE_DIR} FILES ${SOURCES})

target_link_libraries(${PROJECT_NAME}  PRIVATE
OSG::OSG
)