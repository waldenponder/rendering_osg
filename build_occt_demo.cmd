@echo off
setlocal
cd /d "%~dp0"
if not defined OSG_SOURCE_DIR set "OSG_SOURCE_DIR=F:\OpenSceneGraph"
if not defined OCCT_BUILD_DIR set "OCCT_BUILD_DIR=E:\OCCT\build-vs2026-x64"
if not defined VS_ROOT set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
call "%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
set "NINJA=%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

rem Build matching Release OSG libraries without changing its source checkout.
cmake -S "%OSG_SOURCE_DIR%" -B out/osg-release -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_OSG_APPLICATIONS=OFF -DBUILD_OSG_EXAMPLES=OFF
if errorlevel 1 exit /b %errorlevel%
cmake --build out/osg-release --target osgViewer osgGA osgDB osgUtil --parallel 12
if errorlevel 1 exit /b %errorlevel%

cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DBUILD_LEGACY_DEMO=ON -DBUILD_OCCT_DEMO=ON -DOSG_ROOT="%OSG_SOURCE_DIR%" -DOSG_RELEASE_BUILD_DIR="%CD%/out/osg-release" -DOCCT_BUILD_DIR="%OCCT_BUILD_DIR%"
if errorlevel 1 exit /b %errorlevel%
cmake --build out/build/x64-Release --target occt_primitives --parallel 12
if errorlevel 1 exit /b %errorlevel%
out\build\x64-Release\examples\occt_primitives\occt_primitives.exe --check
exit /b %errorlevel%
