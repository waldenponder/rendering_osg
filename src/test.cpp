// custom_drawable.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "common/common.h"
#include "osgDB/ReadFile"
#include "pch.h"
#include <osg/KdTree>
#include <osg/io_utils>
#include <random>

#include "BatchSystem.h"
#include "DynamicGrid .h"
#include "GridUpdateCallback.h"
#include "ThreeDimManipulator.h"
#include "manager/MeshManager.h"
#include "manager/TransformManager.h"
#include "test_ecs2.h"
#include "test_jobsystem.h"

#define WINDOWSIZE 8

//------------------------------------------------------------------------------------------
class MVPCallback : public osg::Uniform::Callback {
  public:
    MVPCallback(osg::Camera *camera) : mCamera(camera) {}

    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        osg::Matrix modelView = mCamera->getViewMatrix();
        osg::Matrix projectM = mCamera->getProjectionMatrix();
        uniform->set(modelView * projectM);
    }

  private:
    osg::Camera *mCamera;
};

// https://blog.csdn.net/qq_16123279/article/details/82463266

osg::Geometry *createLine2(const std::vector<osg::Vec3d> &allPTs,
                           const std::vector<osg::Vec3d> &colors, osg::Camera *camera) {
    cout << "osg::getGLVersionNumber" << osg::getGLVersionNumber() << endl;

    // 传递给shader
    osg::ref_ptr<osg::Vec3Array> a_color = new osg::Vec3Array;

    int nCount = allPTs.size();

    osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> a_pos = new osg::Vec3Array;

    for (int i = 0; i < allPTs.size(); i++) {
        a_pos->push_back(allPTs[i]);
    }

    osg::ref_ptr<osg::ElementBufferObject> ebo = new osg::ElementBufferObject;
    osg::ref_ptr<osg::DrawElementsUInt> indices =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);

    // std::default_random_engine eng(time(NULL));
    // std::uniform_real_distribution<float> rand(.3, 1.);

    for (unsigned int i = 0; i < allPTs.size(); i++) {
        indices->push_back(i);
        a_color->push_back(colors[i]);
    }

    indices->setElementBufferObject(ebo);
    pGeometry->addPrimitiveSet(indices.get());
    pGeometry->setUseVertexBufferObjects(true); // 不启用VBO的话，图元重启没效果

    osg::StateSet *ss = pGeometry->getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::LineWidth(2), osg::StateAttribute::ON);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    //------------------------osg::Program-----------------------------
    osg::Program *program = new osg::Program;
    program->setName("LINESTRIPE");
    program->addShader(
        osgDB::readShaderFile(osg::Shader::VERTEX, shader_dir() + "/line_stripe2.vert"));
    program->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT,
                                             shader_dir() + "/line_stripe2.frag"));

    ss->setAttributeAndModes(program, osg::StateAttribute::ON);

    //-----------attribute  addBindAttribLocation
    // pGeometry->setVertexArray(a_pos);
    pGeometry->setVertexAttribArray(0, a_pos, osg::Array::BIND_PER_VERTEX);
    pGeometry->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
    program->addBindAttribLocation("a_pos", 0);

    pGeometry->setVertexAttribArray(1, a_color, osg::Array::BIND_PER_VERTEX);
    pGeometry->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
    program->addBindAttribLocation("a_color", 1);

    //-----------------------------------------------uniform
    osg::Uniform *u_MVP(new osg::Uniform(osg::Uniform::FLOAT_MAT4, "u_MVP"));
    u_MVP->setUpdateCallback(new MVPCallback(camera));
    ss->addUniform(u_MVP);

    return pGeometry.release();
}

osg::Node *create_lines(osgViewer::Viewer &view) {
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    vector<osg::Vec3d> PTs, COLORs;

    std::default_random_engine eng(time(NULL));
    std::uniform_real_distribution<float> rand(.3, 1.);

    const float SIZE = 100;

    vector<osg::Vec3d> colors;
    colors.push_back(osg::Vec3(1, 0, 0));
    colors.push_back(osg::Vec3(0, 1, 0));
    colors.push_back(osg::Vec3(0, 0, 1));
    colors.push_back(osg::Vec3(1, 1, 0));
    colors.push_back(osg::Vec3(1, 1, 1));

    colors.push_back(osg::Vec3(1, 0, 0));
    colors.push_back(osg::Vec3(0, 1, 0));
    colors.push_back(osg::Vec3(0, 0, 1));
    colors.push_back(osg::Vec3(1, 1, 0));
    colors.push_back(osg::Vec3(1, 0, 1));
    colors.push_back(osg::Vec3(1, 1, 1));
    colors.push_back(osg::Vec3(0, 0, 1));

    PTs.push_back(osg::Vec3(0, 0, 0));
    PTs.push_back(osg::Vec3(100, 0, 0));
    PTs.push_back(osg::Vec3(50, 100, 0));
    PTs.push_back(osg::Vec3(0, 0, 0));

    osg::Geometry *n = createLine2(PTs, colors, view.getCamera());
    n->setName("LINE1");
    geode->addDrawable(n);

    osg::Uniform *uniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "u_color");
    uniform->set(osg::Vec4(1, 1, 0, 1.));
    n->getOrCreateStateSet()->addUniform(uniform);

    return geode.release();
}

float random_float_modern(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min, max);

    return dist(gen);
}

void test_ecs(osg::Group *root) {
    float sz = 500;

    std::vector<Entity> entities;
    auto t1 = clock();

    for (int i = 0; i < 20000; i++) {
        Entity e1 = entity_manager.create();
        entities.push_back(e1);

        // MeshData data1 = get_mesh_func(e1);
        // mesh_manager.set_mesh(e1, data1);

        float r1 = random_float_modern(-sz, sz);
        float r2 = random_float_modern(-sz, sz);

        auto mt1 = osg::Matrix::translate(osg::Vec3f(r1, r2, 0));
        transform_manager.set_local(e1, mt1);
#if 0
            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            osg::ref_ptr<osg::Geometry> geometry = createGeometry(data1);
      
            osg::MatrixTransform *mt = new osg::MatrixTransform;
            auto mat = transform_manager.get_world(e1);
            mt->setMatrix(mat);
      
            geode->addDrawable(geometry);
            mt->addChild(geode);
            root->addChild(mt);
#endif
    }

    auto t2 = clock();
    cout << "aaaa: " << (t2 - t1) << "\n";

    {
        auto aa = clock();

        for (auto e : entities) {
            MeshData data1 = get_mesh_func(e);
            mesh_manager.set_mesh(e, data1);
        }
        auto bb = clock();
        cout << "get-mesh: " << (bb - aa) << "\n";
    }

    t2 = clock();
    std::vector<Batch> batchs = BatchSystem::computeBatch(entities);

    auto t3 = clock();
    cout << "bbb: " << (t3 - t2) << "\n";

    BatchSystem::applyMatrix(entities);

    auto t4 = clock();
    cout << "ccc: " << (t4 - t3) << "\n";

    std::vector<MeshData> datas = BatchSystem::merge(batchs);

    auto t5 = clock();
    cout << "ddd: " << (t5 - t4) << "\n";

    for (auto &data : datas) {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> geometry = createGeometry(data);
        geode->addDrawable(geometry);
        root->addChild(geode);
    }
    auto t6 = clock();
    cout << "eee: " << (t6 - t5) << "\n";
}

osg::Node *create_instance();

int main() 
{
    osg::setNotifyLevel(osg::DEBUG_INFO);

    osgViewer::Viewer viewer;
    osg::Group *root = new osg::Group;

    root->setUpdateCallback(new GridUpdateCallback(&viewer, root));

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root);
    viewer.setUpViewInWindow(100, 100, 1280, 800);
    auto *camera = viewer.getCamera();
    camera->setSmallFeatureCullingPixelSize(2.0f); // 小对象剔除

    add_event_handler(viewer);

    // view.setCameraManipulator(new ThreeDimManipulator(&view));
    // view.addEventHandler(new PickHandler);
    osg::setNotifyLevel(osg::NotifySeverity::NOTICE);
    viewer.realize();
    return viewer.run();
}
