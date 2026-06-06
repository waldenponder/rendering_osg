#include "GridUpdateCallback.h"
#include "DynamicGrid .h"
#include "osg/Camera"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <osg/Matrix>
#include <osg/Plane>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osg/Viewport>

double step_ = 5.0;
size_t cnt = 0;

osg::Vec4 COLOR1(0.7, 0.7, 0.7, 1);
osg::Vec4 COLOR2(0.5, 0.5, 0.5, 1);

GridUpdateCallback::GridUpdateCallback(osgViewer::Viewer *viewer, osg::Group *root)
    : _viewer(viewer) {
    _grid1 = new DynamicGrid;
    _grid2 = new DynamicGrid;
    osg::Vec3d gridCenter;
    _grid1->updateGrid(gridCenter, 2, 100, COLOR1);
    _grid2->updateGrid(gridCenter, 1, 200, COLOR2);
    _current = 2;

    root->addChild(_grid1);
    root->addChild(_grid2);
}

// 返回大于等于val的最小2的幂
uint32_t nextPowerOf2(uint32_t val) {
    if (val == 0)
        return 1;
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val + 1;
}

double nice_step(double val) {
    if (val <= 0.0)
        return 2;

    if (val > 40)
        return 128;
    if (val > 20)
        return 64;
    if (val > 10)
        return 32;
    if (val > 4)
        return 16;
    if (val > 2)
        return 8;
    if (val > 0.2)
        return 4;
    if (val > 0.1)
        return 2;
    if (val > 0.05)
        return 1;
    //if (val > 0.01)
    //    return 0.02;
    return 2;
}

double stp_ = 10;
bool custom_val = false;

void GridUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv) {
    cnt++;

    osg::Camera *cam = _viewer->getCamera();

    osg::Vec3d gridCenter;

    int width = cam->getViewport()->width();
    int height = cam->getViewport()->height();

    osg::Vec3d worldPt, worldPt2;
    bool res = projectToPlane(cam, osg::Vec2(width / 2, height / 2),
                              osg::Plane(osg::Vec3(0, 0, 1), 0), worldPt);

    bool res2 = projectToPlane(cam, osg::Vec2(width / 2 - 10, height / 2 - 10),
                               osg::Plane(osg::Vec3(0, 0, 1), 0), worldPt2);

    if (!res || !res2)
        return;

    double len = (worldPt - worldPt2).length();

    double ll = nice_step(len);

    if (!custom_val)
        stp_ = ll;

    step_ = std::max(step_, 0.01);

    if (cnt % 100 == 0) {
        std::cout << "len :  " << len << "     , current:  " << _current << "    \n";
    }

    if (!is_equal(stp_, _current)) {
        std::cout << "world _current: " << stp_ << "   , " << _current << "\n";
        _current = stp_;
        _grid1->updateGrid(gridCenter, _current, 100, COLOR1);
        _grid2->updateGrid(gridCenter, _current / 2.0, 200, COLOR2);
    }

    traverse(node, nv);
}