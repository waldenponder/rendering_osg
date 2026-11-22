# OCCT + OpenSceneGraph primitives

OCCT creates three actual BRep solids: a blue box, a green cylinder and an orange cone.
`BRepMesh_IncrementalMesh` tessellates each face; the converter copies positions,
surface normals and triangle indices into `osg::Geometry`, applying face locations
and reversing triangle winding/normals for reversed faces.

The viewer uses a Qt 5.13.2 `QMainWindow` and `QOpenGLWidget`. OSG draws into
the widget's framebuffer through `GraphicsWindowEmbedded`. The fixed-function
materials and lights require desktop OpenGL with compatibility support.

The viewport retains the last rendered frame while idle. After editing scene
data on the GUI thread, call `viewport->markDirty()` to schedule a redraw.
Multiple calls before the next paint produce one frame. Camera interaction,
projection changes and framebuffer resizing request their own redraws.

## Windows x64

From `D:\rendering_osg`, run `build_occt_demo.cmd`.

The example finds the Qt 5.13.2 MSVC 2017 x64 SDK installed at
`D:\Qt\5.13.2\msvc2017_64`. If Qt is installed elsewhere, set
`CMAKE_PREFIX_PATH` to the kit directory before configuring a fresh build.
The script uses the previously compiled OCCT Release build at
`E:\OCCT\build-vs2026-x64` and OSG sources at `F:\OpenSceneGraph`.
It builds only the needed OSG Release targets into `out\osg-release`, then builds
this example into `out\build\x64-Release`. Both executable targets belong to the
top-level CMake project and are enabled by default.

Override `OSG_SOURCE_DIR`, `OCCT_BUILD_DIR` or `VS_ROOT` environment variables
before running the script if these dependencies move. Both libraries and the
example must use a compatible x64 MSVC Release runtime. In the Debug configuration,
the example retains unoptimized application code and debug symbols, while using
`/MD` and `_ITERATOR_DEBUG_LEVEL=0` to match the existing Release libraries. The
original playground continues to use its normal Debug OSG libraries.

In Visual Studio, keep the normal `x64-Debug` or `x64-Release` configuration.
Select `occt_primitives.exe` in the startup target list, then use F5 or Ctrl+F5.
The separate `rendering_osg` target builds without Qt when configured with
`-DBUILD_OCCT_DEMO=OFF`.

Open `D:\rendering_osg\out\build\x64-Release\examples\occt_primitives\occt_primitives.exe`.
The build copies the OCCT and OSG DLLs and runs `windeployqt --release` for
both configurations. Keep the deployed Qt DLLs and the `platforms` directory
next to the executable.

Windows x64 Debug and Release passed the headless geometry check and the
window interaction checks. The latter rendered all three colored solids and
exercised orbit, pan, both zoom controls, reset, statistics, resize,
minimize/restore and Escape. Debug was also exercised at 150% Qt scaling.

## CentOS 7 x86_64

Use CMake 3.18 or newer and a C++17 compiler such as Developer Toolset 9.
Use a Qt 5.13.2 desktop SDK with XCB, an OSG SDK with desktop GL and fixed
function support, and an installed OCCT SDK providing
`BRepLib_ToolTriangulatedShape.hxx` and its modeling libraries. Build these
dependencies for the CentOS 7 system ABI. If the prebuilt Qt SDK requires
newer system libraries, build Qt 5.13.2 qtbase on CentOS 7 with XCB and
`-opengl desktop`.

From the repository root, substitute your three SDK install prefixes below
and use the same compiler environment for configuration and build.

```sh
cmake -S . -B out/occt-linux -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_LEGACY_DEMO=OFF -DBUILD_OCCT_DEMO=ON \
  -DCMAKE_PREFIX_PATH="/opt/qt-5.13.2;/opt/osg;/opt/occt"
cmake --build out/occt-linux --target occt_primitives
viewer=./out/occt-linux/examples/occt_primitives/occt_primitives
"$viewer" --check
QT_QPA_PLATFORM=xcb "$viewer"
```

Launch the last command in an X11 session using the system GL driver.
The repository's `run.sh` launches the original playground with its own
runtime libraries. CentOS 7 runtime verification is pending.

## Controls

- The upper-right button switches between perspective (initial view) and
  orthographic projection. The camera keeps its pose and the scale at the
  orbit center. Objects nearer or farther away show the change in perspective.
- Left drag: orbit.
- Middle drag: pan.
- Mouse wheel: zoom about the surface point under the cursor in either
  projection. Over empty space, zoom about the cursor on the orbit-center plane.
- Right drag: zoom.
- Space: return to the initial view.
- S: show OSG statistics.
- F5: redraw the scene once.
- Escape: close.

Pressing on a solid anchors that point on its surface to the cursor throughout
the drag, including the final release position. Middle-dragging empty space
pans at the current orbit-center depth.

`occt_primitives.exe --check` validates all three OCCT solids and tessellations,
constructs their OSG meshes, prints face/vertex/triangle counts, and exits without
opening a graphics window. This checks geometry conversion; use the viewer to
verify actual OpenGL rendering.
