#pragma once
#include "osgViewer/Viewer"

class GridUpdateCallback : public osg::NodeCallback
{
public:
	GridUpdateCallback(osgViewer::Viewer* viewer)
		: _viewer(viewer)
	{
	}

	void operator()(
		osg::Node* node,
		osg::NodeVisitor* nv) override;

private:
	osgViewer::Viewer* _viewer;
};
