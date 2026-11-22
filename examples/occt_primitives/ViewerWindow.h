#pragma once

#include <QOpenGLWidget>
#include <osg/ref_ptr>

namespace osg { class Node; }
namespace osgViewer { class GraphicsWindowEmbedded; class Viewer; }
class SurfaceDragManipulator;

class SceneWidget final : public QOpenGLWidget
{
public:
    explicit SceneWidget(osg::Node& scene);
    ~SceneWidget() override;

    // Call on the GUI thread after changing scene data. Qt coalesces pending requests.
    void markDirty();

protected:
    void initializeGL() override;
    void resizeGL(int, int) override;
    void paintGL() override;
    void showEvent(QShowEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private:
    QSize framebufferSize() const;
    void syncSize();
    void releaseViewer();

    osg::ref_ptr<osg::Node> scene_;
    osg::ref_ptr<SurfaceDragManipulator> trackball_;
    osg::ref_ptr<osgViewer::Viewer> viewer_;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphics_;
    QMetaObject::Connection contextConnection_;
    int wheelAngle_ = 0;
    bool initializedBefore_ = false;
    bool dirty_ = true;
};

int runViewer(int& argc, char** argv, osg::Node& scene);
