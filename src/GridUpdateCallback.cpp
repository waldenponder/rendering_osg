#include "GridUpdateCallback.h"
#include "DynamicGrid .h"
#include "utils.h"
#include "osg/Camera"
#include <cmath>
#include <osg/Matrix>
#include <osg/Plane>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osg/Viewport>
#include <iostream>

double step_ = 5.0;

size_t cnt = 0;

osg::Vec4 COLOR1(0.7, 0.7, 0.7, 1);
osg::Vec4 COLOR2(0.5, 0.5, 0.5, 1);

static bool screenToGround(osg::Camera *cam, double sx, double sy, osg::Vec3d &out) {
    osg::Viewport *vp = cam->getViewport();

    if (!vp)
        return false;

    osg::Matrixd view = cam->getViewMatrix();

    osg::Matrixd proj = cam->getProjectionMatrix();

    osg::Matrixd inv = osg::Matrixd::inverse(view * proj);

    double nx = (sx - vp->x()) / vp->width() * 2.0 - 1.0;

    double ny = (sy - vp->y()) / vp->height() * 2.0 - 1.0;

    osg::Vec3d nearPoint = osg::Vec3d(nx, ny, -1.0) * inv;

    osg::Vec3d farPoint = osg::Vec3d(nx, ny, 1.0) * inv;

    osg::Vec3d dir = farPoint - nearPoint;

    dir.normalize();

    // 与 z=0 平面求交

    if (fabs(dir.z()) < 1e-6)
        return false;

    double t = -nearPoint.z() / dir.z();

    out = nearPoint + dir * t;

    return true;
}

static double get_projected_len(osg::Camera *cam) {
    osg::Viewport *vp = cam->getViewport();

    osg::Matrixd view = cam->getViewMatrix();

    osg::Matrixd proj = cam->getProjectionMatrix();

    auto pt1 = osg::Vec4(0, 0, 0, 1) * view * proj;
    auto pt2 = osg::Vec4(1, 1, 0, 1) * view * proj;

    return (pt1 - pt2).length();
}

static double calculateAdaptiveStep(osg::Camera *cam) {
    osg::Viewport *vp = cam->getViewport();

    if (!vp)
        return 1.0;

    double cx = vp->width() * 0.5;

    double cy = vp->height() * 0.5;

    osg::Vec3d p1;
    osg::Vec3d p2;

    // 屏幕中心
    if (!screenToGround(cam, cx, cy, p1))
        return 1.0;

    // 右边100像素
    if (!screenToGround(cam, cx + 100.0, cy, p2))
        return 1.0;

    double worldDist = (p2 - p1).length();

    return worldDist;
}

GridUpdateCallback::GridUpdateCallback(osgViewer::Viewer *viewer, osg::Group *root)
    : _viewer(viewer) {
    _grid1 = new DynamicGrid;
    _grid2 = new DynamicGrid;
    osg::Vec3d gridCenter;
    _grid1->updateGrid(gridCenter, 2, 100, COLOR1);
    _grid2->updateGrid(gridCenter, 1, 200, COLOR2);

    root->addChild(_grid1);
    root->addChild(_grid2);
}

static double nice_step(double v) {
    if (v <= 0.0)
        return 2;

    std::vector<int> arr = {8192 * 16, 8192 * 8, 8192 * 4, 8192 * 2, 8192,
                            4096,      2048,     1024,     512,      256,
                            128,       64,       32,       16,       8};

    for (auto val : arr) {
        if (v > val)
            return val;
    }
    return 2;
}

void GridUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv) {
    cnt++;

    osg::Camera *cam = _viewer->getCamera();

    osg::Vec3d eye;
    osg::Vec3d center;
    osg::Vec3d up;

    cam->getViewMatrixAsLookAt(eye, center, up);

    double worldDist = calculateAdaptiveStep(cam);
    worldDist = get_projected_len(cam);

    // std::cout << "worldDist: " << worldDist << "\n";
    osg::Vec3d gridCenter;

    int width = cam->getViewport()->width();
    int height = cam->getViewport()->height();

    osg::Vec3d worldPt;
    bool res = projectToPlane(cam, osg::Vec2(width / 2, height / 2),
                              osg::Plane(osg::Vec3(0, 0, 1), 0), worldPt);

    double stp = nice_step(worldDist);

    // if (stp > _current * 2 || stp < _current * 0.5) {
    //     std::cout << "world _current: " << stp << "   , " << _current << "\n";
    //     _current = stp * 4;
    //     _grid1->updateGrid(gridCenter, _current, 100, COLOR1);
    //     _grid2->updateGrid(gridCenter, _current / 2, 200, COLOR2);
    // }
    //  _grid2->updateGrid(gridCenter, step, 100, COLOR2);

    traverse(node, nv);
}