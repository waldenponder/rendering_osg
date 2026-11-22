#include "SurfaceDragManipulator.h"

#include <osg/View>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <cmath>

using osgGA::GUIActionAdapter;
using osgGA::GUIEventAdapter;

namespace {

// Match the embedded viewer's 30-degree vertical field of view.
const double halfHeightAtUnitDepth = std::tan(osg::DegreesToRadians(15.0));

// At the orbit-center depth both projections have exactly this pixel scale.
// Use the current distance even for multiple zoom/drag events in a single frame.
osg::Vec3d focalPlaneOffset(const GUIEventAdapter& event, const osg::Viewport& viewport,
                            double distance)
{
    const double unitsPerPixel = 2.0 * distance * halfHeightAtUnitDepth / viewport.height();
    return {(event.getX() - viewport.x() - viewport.width() * 0.5) * unitsPerPixel,
            (event.getY() - viewport.y() - viewport.height() * 0.5) * unitsPerPixel, 0.0};
}

std::optional<osg::Vec3d> pickSurface(osg::Node& scene, const osg::Vec3d& origin,
                                      osg::Vec3d direction, osg::Node::NodeMask mask)
{
    const auto& bound = scene.getBound();
    if (!bound.valid()) return {};
    direction.normalize();
    const double extent = (osg::Vec3d(bound.center()) - origin).length() + bound.radius();
    osg::ref_ptr<osgUtil::LineSegmentIntersector> pick =
        new osgUtil::LineSegmentIntersector(origin, origin + direction * extent);
    pick->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);
    osgUtil::IntersectionVisitor visitor(pick);
    visitor.setTraversalMask(mask);
    scene.accept(visitor);
    if (!pick->containsIntersections()) return {};
    const auto hit = pick->getFirstIntersection().getWorldIntersectPoint();
    if ((hit - origin) * direction <= 0.0) return {};
    return hit;
}

} // namespace

void SurfaceDragManipulator::setOrthographic(bool enabled)
{
    orthographic_ = enabled;
    anchor_.reset();
    _thrown = false;
    flushMouseEventStack();
}

void SurfaceDragManipulator::updateCamera(osg::Camera& camera)
{
    TrackballManipulator::updateCamera(camera);
    const auto* viewport = camera.getViewport();
    if (!viewport || viewport->height() <= 0 || viewport->width() <= 0) return;

    constexpr double nearClip = 1.0, farClip = 10000.0;
    const double halfHeight = (orthographic_ ? _distance : nearClip) * halfHeightAtUnitDepth;
    const double halfWidth = halfHeight * viewport->width() / viewport->height();
    if (orthographic_)
        camera.setProjectionMatrixAsOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearClip, farClip);
    else
        camera.setProjectionMatrixAsFrustum(-halfWidth, halfWidth, -halfHeight, halfHeight, nearClip, farClip);
}

std::optional<osg::Vec3d> SurfaceDragManipulator::pickUnderCursor(const osg::Vec3d& focalPoint,
                                                                 osg::Node::NodeMask mask)
{
    if (!getNode()) return {};
    const auto eye = getMatrix().getTrans();
    const auto origin = orthographic_ ? eye + (focalPoint - _center) : eye;
    return pickSurface(*getNode(), origin, focalPoint - origin, mask);
}

bool SurfaceDragManipulator::handleMousePush(const GUIEventAdapter& event, GUIActionAdapter& action)
{
    anchor_.reset();
    auto* view = action.asView();
    auto* camera = view ? view->getCamera() : nullptr;
    if (getNode() && camera && camera->getViewport()) {
        const auto offset = focalPlaneOffset(event, *camera->getViewport(), _distance);
        const auto onFocalPlane = _center + _rotation * offset;
        anchor_ = pickUnderCursor(onFocalPlane, camera->getCullMask());
        if (anchor_) {
            const double pickedDepth = -(*anchor_ * getInverseMatrix()).z();
            _center = getMatrix().getTrans() + _rotation * osg::Vec3d(0, 0, -pickedDepth);
            // Orthographic scale depends on distance. Picking a new pivot must
            // preserve that scale; translating along the optical axis is invisible.
            if (!orthographic_) _distance = pickedDepth;
        } else if (event.getButtonMask() == GUIEventAdapter::MIDDLE_MOUSE_BUTTON) {
            // Empty space can still be panned on the current orbit-center plane.
            anchor_ = onFocalPlane;
        }
    }
    return TrackballManipulator::handleMousePush(event, action);
}

bool SurfaceDragManipulator::handleMouseDrag(const GUIEventAdapter& event, GUIActionAdapter& action)
{
    const bool handled = TrackballManipulator::handleMouseDrag(event, action);
    if (anchor_) {
        const auto& viewport = *action.asView()->getCamera()->getViewport();
        _center = *anchor_ - _rotation * focalPlaneOffset(event, viewport, _distance);
    }
    return handled;
}

bool SurfaceDragManipulator::handleMouseRelease(const GUIEventAdapter& event, GUIActionAdapter& action)
{
    if (!anchor_) return TrackballManipulator::handleMouseRelease(event, action);

    // A release may carry a newer position than the last drag. Stop there instead
    // of replaying the last trackball delta or starting an inertial throw.
    handleMouseDrag(event, action);
    anchor_.reset();
    flushMouseEventStack();
    if (event.getButtonMask()) return handleMousePush(event, action);
    return true;
}

bool SurfaceDragManipulator::handleMouseWheel(const GUIEventAdapter& event, GUIActionAdapter& action)
{
    const auto motion = event.getScrollingMotion();
    if (motion != GUIEventAdapter::SCROLL_UP && motion != GUIEventAdapter::SCROLL_DOWN)
        return false;
    auto* view = action.asView();
    auto* camera = view ? view->getCamera() : nullptr;
    if (!camera || !camera->getViewport() || _distance <= 0.0) return false;

    const auto offset = focalPlaneOffset(event, *camera->getViewport(), _distance);
    const auto focalPoint = _center + _rotation * offset;
    const auto target = anchor_ ? *anchor_ : pickUnderCursor(focalPoint, camera->getCullMask()).value_or(focalPoint);
    const double oldDistance = _distance;
    zoomModel(motion == GUIEventAdapter::SCROLL_UP ? getWheelZoomFactor() : -getWheelZoomFactor(), false);
    // Scale the center and eye about the same world point. Its perspective ray
    // stays fixed, and its orthographic offset scales with the viewport extent.
    _center = target + (_center - target) * (_distance / oldDistance);
    _thrown = false;
    action.requestContinuousUpdate(false);
    action.requestRedraw();
    return true;
}

bool SurfaceDragManipulator::handleKeyDown(const GUIEventAdapter& event, GUIActionAdapter& action)
{
    if (event.getKey() == GUIEventAdapter::KEY_Space) anchor_.reset();
    return TrackballManipulator::handleKeyDown(event, action);
}
