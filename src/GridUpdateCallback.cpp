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

double step_ = 10.0;
size_t cnt = 0;

osg::Vec4 COLOR1(0.7, 0.0, 0.0, 1);
osg::Vec4 COLOR2(0.5, 0.5, 0.5, 1);

class PrintGLInfoCallback : public osg::Camera::DrawCallback {
  public:
    virtual void operator()(osg::RenderInfo &) const {
        static bool once = false;

        if (!once) {
            once = true;

            std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

            std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

            std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
        }
    }
};
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

    _viewer->getCamera()->setFinalDrawCallback(new PrintGLInfoCallback);
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
    // static const double steps[] = {
    //     0.01,  0.02,      0.05,      0.1,       0.2,        0.5,        1,
    //     2,     5,         10,        20,        50,         100,        200,
    //     500,   1000,      2000,      4000,      8000,       16000,      32000,
    //     64000, 64000 * 2, 64000 * 4, 64000 * 8, 64000 * 16, 64000 * 32, 64000 * 64};

    static const double steps[] = {0.01,  0.05,      0.2,       1,         5,
                                   20,    100,       500,       2000,      8000,
                                   32000, 64000 * 2, 64000 * 8, 64000 * 64};

    for (auto s : steps) {
        if (val <= s)
            return s;
    }

    return 1000;
}

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

    if (!res || !res2) {
        _grid1->setNodeMask(0);
        _grid2->setNodeMask(0);
        return;
    }

    _grid1->setNodeMask(~0u);
    _grid2->setNodeMask(~0u);

    double len = (worldPt - worldPt2).length();

    double TMP = nice_step(len);

    if (!custom_val)
        step_ = TMP;

    step_ = 10 * std::max(step_, 0.01);

    if (cnt % 100 == 0) {
        std::cout << "len :  " << len << "     , current:  " << _current << "    \n";
    }

    if (!is_equal(step_, _current)) {
        std::cout << "world _current: " << step_ << "   , " << _current << "\n";
        _current = step_;
        _grid1->updateGrid(gridCenter, _current, 64, COLOR1);
        _grid2->updateGrid(gridCenter, _current / 4.0, 64 * 4, COLOR2);
    }

    //auto val1 = glGetString(GL_VENDOR);
    //if (val1)
    //    std::cout << val1 << std::endl;

    //auto val2 = glGetString(GL_RENDERER);
    //if (val2)
    //    std::cout << val2 << std::endl;

    //auto val3 = glGetString(GL_VERSION);
    //if (val3)
    //    std::cout << val3 << std::endl;

    traverse(node, nv);
}