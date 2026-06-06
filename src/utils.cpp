#include "utils.h"
#include "osg/Camera"
#include "osg/Matrix"
#include "osg/Plane"
#include "osg/Vec2"

#include <iostream>

bool projectToPlane(osg::Camera *cam, const osg::Vec2 &ptScreen, const osg::Plane &plane,
                    osg::Vec3d &worldPt) 
{
    // 1. 拼接变换矩阵：viewport(窗口) * proj * view
    osg::Matrix matWindow = cam->getViewport()->computeWindowMatrix();
    osg::Matrix matProj = cam->getProjectionMatrix();
    osg::Matrix matView = cam->getViewMatrix();

    osg::Matrix M = matView * matProj * matWindow;
    osg::Matrix matInv = osg::Matrix::inverse(M);

    matInv.transpose(matInv);

    // osg::Matrix::inverse(M); // 原M.Inverse()

    osg::Vec3d n = plane.getNormal();
    // 平面：原点+法向量 osg::Plane: ax+by+cz+d=0
    osg::Vec3d planeOrigin = osg::Vec3() + n * plane.distance(osg::Vec3());
    double dis = plane.distance(osg::Vec3());

    // 屏幕坐标Y翻转
    double screenX = ptScreen.x();
    double screenY = ptScreen.y();
    // static_cast<double>(getSize().height() - ptScreen.y());

    // 提取逆矩阵各元素 m(row,col) osg::Matrix: m[r][c]
    double m11 = matInv(0, 0), m12 = matInv(0, 1), m13 = matInv(0, 2), m14 = matInv(0, 3);
    double m21 = matInv(1, 0), m22 = matInv(1, 1), m23 = matInv(1, 2), m24 = matInv(1, 3);
    double m31 = matInv(2, 0), m32 = matInv(2, 1), m33 = matInv(2, 2), m34 = matInv(2, 3);
    double m41 = matInv(3, 0), m42 = matInv(3, 1), m43 = matInv(3, 2), m44 = matInv(3, 3);

    double a = screenX * m11 + screenY * m12 + m14;
    double b = screenX * m21 + screenY * m22 + m24;
    double c = screenX * m31 + screenY * m32 + m34;
    double d = screenX * m41 + screenY * m42 + m44;

    // 平面 D = origin · normal
    double D = planeOrigin * n;

    double A = m13 * n.x() + m23 * n.y() + m33 * n.z() - D * m43;
    double B = D * d - (a * n.x() + b * n.y() + c * n.z());

    if (is_equal_zero(A))
        return false;

    double depth = B / A;
    double px = m11 * screenX + m12 * screenY + m13 * depth + m14;
    double py = m21 * screenX + m22 * screenY + m23 * depth + m24;
    double pz = m31 * screenX + m32 * screenY + m33 * depth + m34;
    double pw = m41 * screenX + m42 * screenY + m43 * depth + m44;

    if (is_equal_zero(pw))
        return false;

    // 齐次归一化
    worldPt.set(px / pw, py / pw, pz / pw);

     auto screen = worldPt * matView * matProj * matWindow;

     double tx1 = ptScreen.x() - screen.x();
     double ty1 = ptScreen.y() - screen.y();

     //std::cout << "check:  " << tx1 << "   , " << ty1 << "   worldPt:  " << worldPt.x()
     //          << ",  " << worldPt.y() << "  , " << worldPt.z() << "\n";

    return true;
}
