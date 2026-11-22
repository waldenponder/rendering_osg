cmake_minimum_required(VERSION 3.18)
project(rendering_osg)

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

option(BUILD_LEGACY_DEMO "Build the existing rendering playground" ON)
option(BUILD_OCCT_DEMO "Build the OCCT to OSG primitives viewer" ON)

if(BUILD_OCCT_DEMO)
    if(WIN32)
        include(cmake/osg.cmake)
    endif()
    add_subdirectory(examples/occt_primitives)
endif()

if(BUILD_LEGACY_DEMO)
include(cmake/osg.cmake)

file(GLOB_RECURSE HEADER_FILES ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.cc ${SOURCE_DIR}/*.c)
set(SOURCES ${HEADER_FILES} ${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

source_group(TREE ${SOURCE_DIR} FILES ${SOURCES})

if (WIN32)
target_link_libraries(${PROJECT_NAME}  PRIVATE
OSG::OSG
${CMAKE_SOURCE_DIR}/lib/OpenGL32.Lib
)
else()
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OSG::OSG
    Threads::Threads 
    ${CMAKE_DL_LIBS}
	${OPENGL_LIBRARIES}   # 添加这一行，链接 OpenGL
)
endif()
endif()
