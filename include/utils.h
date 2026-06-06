#pragma once
#include "osg/Geode"

// 浮点等值判断，OSG常规精度
inline bool isEqual(double val, double eps = 1e-8) { return std::fabs(val) < eps; }

bool projectToPlane(osg::Camera *cam, const osg::Vec2 &ptScreen, const osg::Plane &plane,
                    osg::Vec3d &worldPt);