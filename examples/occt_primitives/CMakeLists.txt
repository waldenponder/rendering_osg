if(WIN32)
    set(qt_sdk_hint "D:/Qt/5.13.2/msvc2017_64")
endif()
find_package(Qt5 5.13.2 EXACT CONFIG REQUIRED COMPONENTS Widgets PATHS ${qt_sdk_hint})
find_package(OpenGL REQUIRED)
add_executable(occt_primitives main.cpp ViewerWindow.cpp ViewerWindow.h
    SurfaceDragManipulator.cpp SurfaceDragManipulator.h)
target_link_libraries(occt_primitives PRIVATE Qt5::Widgets OpenGL::GL)
target_compile_definitions(occt_primitives PRIVATE QT_NO_KEYWORDS)

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
if(WIN32)
    set_target_properties(Qt5::Core Qt5::Gui Qt5::Widgets PROPERTIES MAP_IMPORTED_CONFIG_DEBUG Release)
    set(OCCT_BUILD_DIR "E:/OCCT/build-vs2026-x64" CACHE PATH "Previously compiled OCCT build")
    set(OCCT_LIBRARY_DIR "${OCCT_BUILD_DIR}/win64/vc14/lib" CACHE PATH "OCCT library directory")
    set(OCCT_RUNTIME_DIR "${OCCT_BUILD_DIR}/win64/vc14/bin" CACHE PATH "OCCT runtime directory")
    find_path(OCCT_INCLUDE_DIR BRepPrimAPI_MakeBox.hxx
        PATHS "${OCCT_BUILD_DIR}/inc" NO_DEFAULT_PATH REQUIRED)
    target_include_directories(occt_primitives PRIVATE "${OCCT_INCLUDE_DIR}")
    target_compile_definitions(occt_primitives PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN QT_NO_DEBUG)
    target_link_libraries(occt_primitives PRIVATE OSG::Release)
    foreach(toolkit IN LISTS OCCT_TOOLKITS)
        find_library(OCCT_${toolkit}_LIBRARY NAMES ${toolkit}
            PATHS "${OCCT_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
        target_link_libraries(occt_primitives PRIVATE "${OCCT_${toolkit}_LIBRARY}")
        add_custom_command(TARGET occt_primitives POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OCCT_RUNTIME_DIR}/${toolkit}.dll" "$<TARGET_FILE_DIR:occt_primitives>")
    endforeach()
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
    get_target_property(qmake_exe Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(qt_bin "${qmake_exe}" DIRECTORY)
    if(NOT EXISTS "${qt_bin}/windeployqt.exe")
        message(FATAL_ERROR "Qt deployment tool missing: ${qt_bin}/windeployqt.exe")
    endif()
    add_custom_command(TARGET occt_primitives POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${qt_bin};$ENV{PATH}"
            "${qt_bin}/windeployqt.exe" --release --no-translations --no-compiler-runtime
            "$<TARGET_FILE:occt_primitives>" VERBATIM)
else()
    find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgGA osgDB osgUtil)
    find_package(OpenCASCADE CONFIG REQUIRED COMPONENTS FoundationClasses ModelingData ModelingAlgorithms)
    target_include_directories(occt_primitives PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS} "${OpenCASCADE_INCLUDE_DIR}")
    target_link_libraries(occt_primitives PRIVATE ${OPENSCENEGRAPH_LIBRARIES} ${OCCT_TOOLKITS})
endif()
