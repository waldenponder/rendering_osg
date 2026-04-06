cmake_minimum_required(VERSION 3.10)
project(rendering_osg)


set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 直接指定要包含的目录
set(INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/osg3.6.5/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/include/common
	${CMAKE_CURRENT_SOURCE_DIR}/include/manager
	${CMAKE_CURRENT_SOURCE_DIR}/include/utils
    # 可以添加更多目录
    # "${CMAKE_CURRENT_SOURCE_DIR}/other/include"
)
message(STATUS "Include directories: ${INCLUDE_DIRS}")

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/osg3.6.5/lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# 添加库文件搜索路径
#target_link_directories(${PROJECT_NAME} PRIVATE ${LIB_DIR})

file(GLOB_RECURSE HEADER_FILES ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.cc ${SOURCE_DIR}/*.c)
set(SOURCES ${HEADER_FILES} ${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

source_group(TREE ${SOURCE_DIR} FILES ${SOURCES})
# source_group(TREE ${SOURCE_DIR} FILES ${HEADER_FILES} PREFIX "Header Files")
# source_group(TREE ${SOURCE_DIR} FILES ${SOURCE_FILES} PREFIX "Source Files")



target_link_libraries(${PROJECT_NAME}
	debug ${LIB_DIR}/OpenThreadsd.lib
    debug ${LIB_DIR}/osgd.lib
	debug ${LIB_DIR}/osgUtild.lib
    debug ${LIB_DIR}/osgDBd.lib
	debug ${LIB_DIR}/osgViewerd.lib
	debug ${LIB_DIR}/osgGAd.lib
	
    optimized ${LIB_DIR}/OpenThreads.lib
    optimized ${LIB_DIR}/osg.lib
	optimized ${LIB_DIR}/osgUtil.lib
    optimized ${LIB_DIR}/osgDB.lib
	optimized ${LIB_DIR}/osgViewer.lib
	optimized ${LIB_DIR}/osgGA.lib
)