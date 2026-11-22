#include "ViewerWindow.h"
#include "SurfaceDragManipulator.h"

#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPushButton>
#include <QSurfaceFormat>
#include <QWheelEvent>

namespace {

unsigned int osgButton(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton: return 1;
    case Qt::MiddleButton: return 2;
    case Qt::RightButton: return 3;
    default: return 0;
    }
}

class ViewportStats final : public osgViewer::StatsHandler
{
public:
    ViewportStats()
    {
        _font.clear(); // Use osgText's built-in font on both platforms.
        getCamera()->setDrawBuffer(GL_COLOR_ATTACHMENT0);
        getCamera()->setReadBuffer(GL_COLOR_ATTACHMENT0);
    }
};

} // namespace

SceneWidget::SceneWidget(osg::Node& scene) : scene_(&scene), trackball_(new SurfaceDragManipulator)
{
    setFocusPolicy(Qt::StrongFocus);
    // Qt may composite the widget again without any change to the 3D scene.
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    trackball_->setHomePosition({9, -23, 14}, {0, 0, 1}, {0, 0, 1});

    auto* projectionButton = new QPushButton(QStringLiteral("切换到正交"), this);
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(projectionButton, 0, Qt::AlignTop | Qt::AlignRight);
    connect(projectionButton, &QPushButton::clicked, this, [this, projectionButton] {
        const bool orthographic = !trackball_->isOrthographic();
        trackball_->setOrthographic(orthographic);
        if (viewer_) viewer_->requestContinuousUpdate(false); // Switching ends a trackball throw.
        projectionButton->setText(orthographic ? QStringLiteral("切换到透视")
                                               : QStringLiteral("切换到正交"));
        setFocus();
        markDirty();
    });
}

SceneWidget::~SceneWidget() { releaseViewer(); }

void SceneWidget::markDirty()
{
    dirty_ = true;
    update();
}

void SceneWidget::initializeGL()
{
    contextConnection_ = connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                                 this, &SceneWidget::releaseViewer, Qt::DirectConnection);
    viewer_ = new osgViewer::Viewer;
    viewer_->setReleaseContextAtEndOfFrameHint(false);
    const QSize size = framebufferSize();
    graphics_ = viewer_->setUpViewerAsEmbeddedInWindow(0, 0, size.width(), size.height());
    auto* camera = viewer_->getCamera();
    // Qt binds a color attachment, even when the top-level window is double-buffered.
    camera->setDrawBuffer(GL_COLOR_ATTACHMENT0);
    camera->setReadBuffer(GL_COLOR_ATTACHMENT0);
    camera->setClearColor({0.09f, 0.115f, 0.16f, 1});
    viewer_->setSceneData(scene_);
    viewer_->setCameraManipulator(trackball_, !initializedBefore_);
    viewer_->addEventHandler(new ViewportStats);
    viewer_->realize();
    viewer_->init();
    graphics_->getEventQueue()->windowResize(0, 0, size.width(), size.height());
    initializedBefore_ = true;
    dirty_ = true;
}

void SceneWidget::resizeGL(int, int)
{
    syncSize();
    dirty_ = true; // Qt has allocated a new framebuffer.
}

void SceneWidget::paintGL()
{
    syncSize();
    if (!dirty_ && !graphics_->checkEvents() && !viewer_->getRequestRedraw()
        && !viewer_->getRequestContinousUpdate()) return;

    // Dispatch queued input first. A key release, for example, needs no scene redraw.
    viewer_->advance();
    viewer_->eventTraversal();
    if (dirty_ || viewer_->getRequestRedraw() || viewer_->getRequestContinousUpdate()
        || trackball_->getInverseMatrix() != viewer_->getCamera()->getViewMatrix()) {
        dirty_ = false;
        graphics_->setDefaultFboId(defaultFramebufferObject());
        viewer_->updateTraversal();
        viewer_->renderingTraversals();
    }
    if (viewer_->getRequestContinousUpdate()) update();
}

void SceneWidget::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    update(); // Deliver work queued while the widget was hidden.
}

void SceneWidget::mousePressEvent(QMouseEvent* event)
{
    const auto button = osgButton(event->button());
    if (graphics_ && button) {
        const auto p = event->localPos() * devicePixelRatioF();
        graphics_->getEventQueue()->mouseButtonPress(p.x(), p.y(), button);
        update();
    }
}

void SceneWidget::mouseReleaseEvent(QMouseEvent* event)
{
    const auto button = osgButton(event->button());
    if (graphics_ && button) {
        const auto p = event->localPos() * devicePixelRatioF();
        graphics_->getEventQueue()->mouseButtonRelease(p.x(), p.y(), button);
        update();
    }
}

void SceneWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (graphics_) {
        const auto p = event->localPos() * devicePixelRatioF();
        graphics_->getEventQueue()->mouseMotion(p.x(), p.y());
        update();
    }
}

void SceneWidget::wheelEvent(QWheelEvent* event)
{
    if (!graphics_) return;
    wheelAngle_ += event->angleDelta().y();
    const int steps = wheelAngle_ / 120;
    wheelAngle_ %= 120;
    if (steps == 0) return;
    // OSG copies the last pointer event's position onto each scroll event.
    const auto p = event->posF() * devicePixelRatioF();
    auto* queue = graphics_->getEventQueue();
    queue->mouseMotion(p.x(), p.y());
    const auto direction = steps > 0 ? osgGA::GUIEventAdapter::SCROLL_UP
                                     : osgGA::GUIEventAdapter::SCROLL_DOWN;
    for (int i = 0; i < qAbs(steps); ++i)
        queue->mouseScroll(direction);
    update();
}

void SceneWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        window()->close();
    } else if (event->key() == Qt::Key_F5) {
        markDirty();
    } else if (graphics_ && !event->text().isEmpty()) {
        graphics_->getEventQueue()->keyPress(event->text().at(0).unicode());
        update();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}

void SceneWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (graphics_ && !event->text().isEmpty()) {
        graphics_->getEventQueue()->keyRelease(event->text().at(0).unicode());
        update();
    } else {
        QOpenGLWidget::keyReleaseEvent(event);
    }
}

QSize SceneWidget::framebufferSize() const
{
    return (size() * devicePixelRatioF()).expandedTo(QSize(1, 1));
}

void SceneWidget::syncSize()
{
    const auto size = framebufferSize();
    const auto* traits = graphics_->getTraits();
    if (size.width() == traits->width && size.height() == traits->height) return;
    // OSG updates all attached viewports and adjusts the camera projection here.
    graphics_->resized(0, 0, size.width(), size.height());
    graphics_->getEventQueue()->windowResize(0, 0, size.width(), size.height());
}

void SceneWidget::releaseViewer()
{
    if (!viewer_) return;
    disconnect(contextConnection_);
    makeCurrent();
    viewer_->releaseGLObjects(graphics_->getState());
    viewer_ = nullptr;
    graphics_ = nullptr;
    doneCurrent();
}

int runViewer(int& argc, char** argv, osg::Node& scene)
{
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(2, 1); // The scene uses OSG fixed-function materials and lighting.
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle(QStringLiteral("OCCT + OpenSceneGraph | Box - Cylinder - Cone"));
    auto* viewport = new SceneWidget(scene);
    window.setCentralWidget(viewport);
    window.resize(1280, 800);
    window.show();
    viewport->setFocus();
    return app.exec();
}
