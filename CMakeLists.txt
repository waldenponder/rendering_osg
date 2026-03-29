cmake_minimum_required(VERSION 3.10)
project(rendering_osg)

# ==================== 递归搜索源文件函数 ====================
function(collect_sources OUT_VAR SEARCH_PATH)
    set(SOURCE_FILES "")
    file(GLOB_RECURSE SRC_FILES 
        "${SEARCH_PATH}/*.cpp"
        "${SEARCH_PATH}/*.cc"
        "${SEARCH_PATH}/*.c"
        "${SEARCH_PATH}/*.cxx"
        "${SEARCH_PATH}/*.c++"
		"${SEARCH_PATH}/*.h"
		"${SEARCH_PATH}/*.hpp"
    )
    set(${OUT_VAR} ${SRC_FILES} PARENT_SCOPE)
endfunction()

# ==================== 使用示例 ====================
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
collect_sources(SOURCES ${SOURCE_DIR})
message(STATUS "Found ${SOURCES}")

# 直接指定要包含的目录
set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/osg3.6.5/include"
    # 可以添加更多目录
    # "${CMAKE_CURRENT_SOURCE_DIR}/other/include"
)
message(STATUS "Include directories: ${INCLUDE_DIRS}")

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/osg3.6.5/lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 添加头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# 添加库文件搜索路径
#target_link_directories(${PROJECT_NAME} PRIVATE ${LIB_DIR})

file(GLOB_RECURSE HEADER_FILES ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.cc ${SOURCE_DIR}/*.c)
set(SOURCES ${HEADER_FILES} ${SOURCE_FILES})

# 分别生成过滤器
# source_group(TREE ${SOURCE_DIR} FILES ${SOURCES})
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