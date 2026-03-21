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


# 设置库目录
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/osg3.6.5/lib")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# 根据构建类型选择不同的库
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(STATUS "Using Debug libraries")
    set(LIBRARIES 
        osgd
        osgDBd
        osgUId
		osgTextd
		osgViewerd
		osgGAd
    )
else()
    message(STATUS "Using Release libraries")

    set(LIBRARIES 
        osg
        osgDB
        osgUI
		osgText
		osgViewer
		osgGA
    )
endif()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 添加头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# 添加库文件搜索路径
target_link_directories(${PROJECT_NAME} PRIVATE ${LIB_DIR})

# 链接库文件
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})