#pragma once

#include <osgGA/TrackballManipulator>
#include <optional>

class SurfaceDragManipulator final : public osgGA::TrackballManipulator
{
public:
    bool isOrthographic() const { return orthographic_; }
    void setOrthographic(bool enabled);
    void updateCamera(osg::Camera& camera) override;

protected:
    bool handleMousePush(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override;
    bool handleMouseDrag(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override;
    bool handleMouseRelease(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override;
    bool handleMouseWheel(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override;
    bool handleKeyDown(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override;

private:
    std::optional<osg::Vec3d> pickUnderCursor(const osg::Vec3d& focalPoint, osg::Node::NodeMask mask);
    std::optional<osg::Vec3d> anchor_;
    bool orthographic_ = false;
};
