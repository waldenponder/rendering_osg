# 防止重复包含
if(TARGET OSG::OSG)
    return()
endif()

set(OSG_ROOT "F:/OpenSceneGraph")
set(OSG_INCLUDE_DIR "${OSG_ROOT}/include")
set(OSG_LIB_DIR "${OSG_ROOT}/build/lib")

message(STATUS "root dir　　CMAKE_SOURCE_DIR 　${CMAKE_SOURCE_DIR}" )
message(STATUS "current dir　CMAKE_CURRENT_LIST_DIR 　${CMAKE_CURRENT_LIST_DIR}" )

add_library(OSG::OSG INTERFACE IMPORTED)

target_include_directories(OSG::OSG INTERFACE
    ${OSG_INCLUDE_DIR}
)

target_link_libraries(OSG::OSG INTERFACE
    debug ${OSG_LIB_DIR}/OpenThreadsd.lib
    debug ${OSG_LIB_DIR}/osgd.lib
    debug ${OSG_LIB_DIR}/osgUtild.lib
    debug ${OSG_LIB_DIR}/osgDBd.lib
    debug ${OSG_LIB_DIR}/osgViewerd.lib
    debug ${OSG_LIB_DIR}/osgGAd.lib

    optimized ${OSG_LIB_DIR}/OpenThreads.lib
    optimized ${OSG_LIB_DIR}/osg.lib
    optimized ${OSG_LIB_DIR}/osgUtil.lib
    optimized ${OSG_LIB_DIR}/osgDB.lib
    optimized ${OSG_LIB_DIR}/osgViewer.lib
    optimized ${OSG_LIB_DIR}/osgGA.lib
)