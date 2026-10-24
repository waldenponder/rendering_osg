# OCCT + OpenSceneGraph primitives

OCCT creates three actual BRep solids: a blue box, a green cylinder and an orange cone.
`BRepMesh_IncrementalMesh` tessellates each face; the converter copies positions,
surface normals and triangle indices into `osg::Geometry`, applying face locations
and reversing triangle winding/normals for reversed faces.

## Build on this machine

From `D:\rendering_osg`, run `build_occt_demo.cmd`.

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

## Run

In Visual Studio, keep the normal `x64-Debug` or `x64-Release` configuration.
After CMake generation completes, select either `rendering_osg.exe` or
`occt_primitives.exe` in the startup target list, then use F5 or Ctrl+F5.
There is no separate OCCT configuration.

Open `D:\rendering_osg\out\build\x64-Release\examples\occt_primitives\occt_primitives.exe`.
Required OCCT and OSG DLLs are copied beside the executable during the build.

- Left drag: orbit.
- Middle drag: pan.
- Mouse wheel or right drag: zoom.
- Space: return to the initial view.
- S: show OSG statistics.
- Escape: close.

`occt_primitives.exe --check` validates all three OCCT solids and tessellations,
constructs their OSG meshes, prints face/vertex/triangle counts, and exits without
opening a graphics window. This checks geometry conversion; use the viewer to
verify actual OpenGL rendering.
