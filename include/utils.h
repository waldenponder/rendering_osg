#pragma once
#include "osg/Geode"

// 浮点等值判断，OSG常规精度
inline bool is_equal_zero(double val, double eps = 1e-8) { return std::fabs(val) < eps; }

inline bool is_equal(double val, double val2, double eps = 1e-8) {
    return std::fabs(val - val2) < eps;
}

bool projectToPlane(osg::Camera *cam, const osg::Vec2 &ptScreen, const osg::Plane &plane,
                    osg::Vec3d &worldPt);