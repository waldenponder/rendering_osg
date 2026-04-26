#pragma once
#include <osgGA/OrbitManipulator>
#include "osgViewer/Viewer"

class ThreeDimManipulator : public osgGA::OrbitManipulator
{
public:
	ThreeDimManipulator(osgViewer::Viewer* viewer);
	~ThreeDimManipulator();

public:
	bool handleMousePush(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
	bool handleMouseRelease(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
	bool handleMouseDrag(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
	bool handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
	bool handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
	bool handleKeyUp(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

	void rotateTrackball(const float px0, const float py0, const float px1, const float py1,
	                     const float scale) override;
	bool performMovement() override;

private:
	osg::Camera* getCamera() const;
	void focus();
	void rectZoomOrth();
	void rectZoomPersp();
	void toggleCameraModel();

	osg::Vec2 _preMousePt;
	int _buttonType = -1;
	int _keyType = -1;
	float _zoomFactor = 1;
	osgViewer::Viewer* _viewer;
};
