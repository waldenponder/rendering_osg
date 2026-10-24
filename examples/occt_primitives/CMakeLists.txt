set(OCCT_BUILD_DIR "E:/OCCT/build-vs2026-x64" CACHE PATH "Previously compiled OCCT build")
set(OCCT_LIBRARY_DIR "${OCCT_BUILD_DIR}/win64/vc14/lib" CACHE PATH "OCCT library directory")
set(OCCT_RUNTIME_DIR "${OCCT_BUILD_DIR}/win64/vc14/bin" CACHE PATH "OCCT runtime directory")

find_path(OCCT_INCLUDE_DIR BRepPrimAPI_MakeBox.hxx
    PATHS "${OCCT_BUILD_DIR}/inc" NO_DEFAULT_PATH REQUIRED)

add_executable(occt_primitives main.cpp)
target_compile_definitions(occt_primitives PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(occt_primitives PRIVATE "${OCCT_INCLUDE_DIR}")
if(WIN32)
    target_link_libraries(occt_primitives PRIVATE OSG::Release)
else()
    target_link_libraries(occt_primitives PRIVATE OSG::OSG)
endif()

# Keep this example on the same Release CRT as the existing OCCT build.
if(MSVC)
    target_compile_options(occt_primitives PRIVATE /utf-8)
    set_property(TARGET occt_primitives PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    # Debug our application code while matching the prebuilt Release SDK ABI.
    target_compile_definitions(occt_primitives PRIVATE _ITERATOR_DEBUG_LEVEL=0 NDEBUG)
    target_compile_options(occt_primitives PRIVATE /U_DEBUG /Z7)
endif()
set(OCCT_TOOLKITS TKPrim TKMesh TKTopAlgo TKShHealing TKGeomAlgo TKBRep
    TKGeomBase TKG3d TKG2d TKMath TKernel)
foreach(toolkit IN LISTS OCCT_TOOLKITS)
    find_library(OCCT_${toolkit}_LIBRARY NAMES ${toolkit}
        PATHS "${OCCT_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
    target_link_libraries(occt_primitives PRIVATE "${OCCT_${toolkit}_LIBRARY}")
    if(WIN32)
        add_custom_command(TARGET occt_primitives POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OCCT_RUNTIME_DIR}/${toolkit}.dll" "$<TARGET_FILE_DIR:occt_primitives>")
    endif()
endforeach()
if(WIN32)
    target_link_libraries(occt_primitives PRIVATE opengl32)
    foreach(component OpenThreads osg osgUtil osgDB osgViewer osgGA osgText)
        file(GLOB runtime_dlls "${OSG_RELEASE_BUILD_DIR}/bin/*${component}.dll")
        if(NOT runtime_dlls)
            message(FATAL_ERROR "Missing Release ${component} DLL in ${OSG_RELEASE_BUILD_DIR}/bin. Run build_occt_demo.cmd first.")
        endif()
        add_custom_command(TARGET occt_primitives POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${runtime_dlls} "$<TARGET_FILE_DIR:occt_primitives>"
            COMMAND_EXPAND_LISTS)
    endforeach()
endif()
