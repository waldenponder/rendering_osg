cmake_minimum_required(VERSION 3.21)
project(step_mesh_bench LANGUAGES CXX)

find_package(OpenCASCADE CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED COMPONENTS tbb)
add_executable(step_mesh_bench main.cpp)
target_compile_features(step_mesh_bench PRIVATE cxx_std_17)
target_include_directories(step_mesh_bench PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
if(TARGET TKDESTEP)
    set(step_toolkit TKDESTEP)
else()
    set(step_toolkit TKSTEP)
endif()
target_link_libraries(step_mesh_bench PRIVATE TBB::tbb ${step_toolkit} TKXSBase
                      TKMesh TKTopAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKernel)
if(WIN32)
    target_compile_definitions(step_mesh_bench PRIVATE NOMINMAX)
    add_custom_command(TARGET step_mesh_bench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:step_mesh_bench>
                $<TARGET_FILE_DIR:step_mesh_bench> COMMAND_EXPAND_LISTS VERBATIM)
endif()
