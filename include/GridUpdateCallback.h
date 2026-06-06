#pragma once
#include "DynamicGrid .h"
#include "osgViewer/Viewer"

class GridUpdateCallback : public osg::NodeCallback {
  public:
    GridUpdateCallback(osgViewer::Viewer *viewer, osg::Group *root);
  

    void operator()(osg::Node *node, osg::NodeVisitor *nv) override;

  private:
    osgViewer::Viewer *_viewer;

    osg::ref_ptr<DynamicGrid> _grid1;
    osg::ref_ptr<DynamicGrid> _grid2;

    double _current = -1;
};
