# 防止重复包含
if(TARGET OSG::OSG)
    return()
endif()

if (WIN32)
set(OSG_ROOT "F:/OpenSceneGraph" CACHE PATH "OpenSceneGraph source or SDK root")
set(OSG_BUILD_DIR "${OSG_ROOT}/build" CACHE PATH "OpenSceneGraph build directory")
set(OSG_RELEASE_BUILD_DIR "${CMAKE_SOURCE_DIR}/out/osg-release" CACHE PATH "OpenSceneGraph Release build directory")
set(OSG_INCLUDE_DIR "${OSG_ROOT}/include")
set(OSG_LIB_DIR "${OSG_BUILD_DIR}/lib")

# The OCCT example uses the existing Release SDK in every IDE configuration.
# Keep this interface separate from the playground's Debug OSG dependencies.
add_library(OSG::Release INTERFACE IMPORTED)
target_include_directories(OSG::Release INTERFACE
    "${OSG_RELEASE_BUILD_DIR}/include" "${OSG_INCLUDE_DIR}")
foreach(component OpenThreads osg osgUtil osgDB osgViewer osgGA)
    target_link_libraries(OSG::Release INTERFACE "${OSG_RELEASE_BUILD_DIR}/lib/${component}.lib")
endforeach()

message(STATUS "root dir　　CMAKE_SOURCE_DIR 　${CMAKE_SOURCE_DIR}" )
message(STATUS "current dir　CMAKE_CURRENT_LIST_DIR 　${CMAKE_CURRENT_LIST_DIR}" )

add_library(OSG::OSG INTERFACE IMPORTED)

target_include_directories(OSG::OSG INTERFACE
    "$<$<CONFIG:Debug>:${OSG_BUILD_DIR}/include>"
    "$<$<NOT:$<CONFIG:Debug>>:${OSG_RELEASE_BUILD_DIR}/include>"
    ${OSG_INCLUDE_DIR}
)
target_link_libraries(OSG::OSG INTERFACE
    debug ${OSG_LIB_DIR}/OpenThreadsd.lib
    debug ${OSG_LIB_DIR}/osgd.lib
    debug ${OSG_LIB_DIR}/osgUtild.lib
    debug ${OSG_LIB_DIR}/osgDBd.lib
    debug ${OSG_LIB_DIR}/osgViewerd.lib
    debug ${OSG_LIB_DIR}/osgGAd.lib

    optimized OSG::Release
)

else()
set(OSG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../OpenSceneGraph")
set(OSG_BUILD_DIR "${OSG_ROOT}/build")

set(OSG_INCLUDE_DIR "${OSG_ROOT}/include")
set(OSG_LIB_DIR "${OSG_BUILD_DIR}/lib")

message(STATUS "OSG Root Dir: ${OSG_ROOT}")
message(STATUS "OSG Include Dir: ${OSG_INCLUDE_DIR}")
message(STATUS "OSG Lib Dir: ${OSG_LIB_DIR}")

add_library(OSG::OSG INTERFACE IMPORTED)

target_include_directories(OSG::OSG INTERFACE
    ${OSG_INCLUDE_DIR}
)

target_link_libraries(OSG::OSG INTERFACE
	${OSG_LIB_DIR}/libOpenThreads.so
	${OSG_LIB_DIR}/libosg.so
	${OSG_LIB_DIR}/libosgUtil.so
	${OSG_LIB_DIR}/libosgDB.so
	${OSG_LIB_DIR}/libosgViewer.so
	${OSG_LIB_DIR}/libosgGA.so
)
endif()
