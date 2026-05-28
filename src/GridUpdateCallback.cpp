#include "GridUpdateCallback.h"

#include "DynamicGrid .h"
#include "osg/Camera"


void GridUpdateCallback::operator()(
    osg::Node* node,
    osg::NodeVisitor* nv)
{
    DynamicGrid* grid =
        dynamic_cast<DynamicGrid*>(node);

    if (!grid)
    {
        traverse(node, nv);
        return;
    }

    osg::Camera* cam = _viewer->getCamera();

    osg::Vec3d eye, center, up;

    cam->getViewMatrixAsLookAt(
        eye,
        center,
        up);

    double dist = (center - eye).length();
      //  fabs(eye.z());

    double base =
        pow(10.0,
            floor(log10(dist)));
    //---
    double step = std::max(base, 1.0);

    grid->updateGrid(
        osg::Vec3d(),
        step,
        50);

    traverse(node, nv);
}
