#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>

const char* vertCode = {
    "uniform sampler2D defaultTex;\n"
    "const float PI2 = 6.2831852;\n"
    "void main()\n"
    "{\n"
    "    float r = float(gl_InstanceID) / 256.0;\n"
    "    vec2 uv = vec2(fract(r), floor(r) / 256.0);\n"
    "    vec4 pos = gl_Vertex + vec4(uv.s * 384.0, 32.0 * sin(uv.s * PI2), "
    "uv.t * 384.0, 1.0);\n"
    // "    gl_FrontColor = texture2D(defaultTex, uv);\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * pos;\n"
    "}\n"
};

#include "CollectDrawableVisitor.h"

osg::Node* createInstancedGeometry(unsigned int numInstances)
{
    //osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(4);
    //(*vertices)[0].set(-0.5f, 0.0f, -0.5f);
    //(*vertices)[1].set(0.5f, 0.0f, -0.5f);
    //(*vertices)[2].set(0.5f, 0.0f, 0.5f);
    //(*vertices)[3].set(-0.5f, 0.0f, 0.5f);


    auto node = osgDB::readNodeFile("I:/cylinder.obj");
    CollectDrawableVisitor visitor;
    node->accept(visitor);

    osg::Geode *res = new osg::Geode;

    //osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    //texture->setImage(osgDB::readImageFile("I:/osg128.png"));
    //texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    //texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    //geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    // geom->getOrCreateStateSet()->addUniform(new osg::Uniform("defaultTex", 0));

    for (auto geom2 : visitor.getDrawables())
    {
      osg::Geometry *geom = new osg::Geometry;
      geom->setUseDisplayList(false);
      geom->setUseVertexBufferObjects(true);
      geom->setVertexArray(geom2->getVertexArray());
        auto primitive = geom2->getPrimitiveSet(0);
      primitive->setNumInstances(numInstances);
        geom->addPrimitiveSet(primitive);
          //new osg::DrawElementsUShort(GL_TRIANGLES, 0, vertices->asVector().size(), numInstances));
      geom->setInitialBound(
          osg::BoundingBox(-1.0f, -32.0f, -1.0f, 192.0f, 32.0f, 192.0f));

      osg::ref_ptr<osg::Program> program = new osg::Program;
      program->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
      geom->getOrCreateStateSet()->setAttributeAndModes(program.get());
      res->addDrawable(geom);
    }
    return res;
}

osg::Node *create_instance() {
    //osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    //geode->addDrawable();

    auto node = createInstancedGeometry(512 * 1000);
    return node;
}
