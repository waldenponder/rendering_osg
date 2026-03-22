#include "MeshManager.h"

CollectDrawableVisitor cv_;

osg::ref_ptr<osg::Geometry> createGeometry(MeshData& data)
{
    if (!data.isValid())
        return nullptr;

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    auto& vert_ = data.vert_;
    auto& color_ = data.color_;
    auto& normal_ = data.normal_;
    auto& indices_ = data.indices_;

    geom->setVertexArray(data.vert_);

    if (color_ && !color_->empty())
    {
        geom->setColorArray(color_, osg::Array::BIND_PER_VERTEX);
    }

    if (normal_ && !normal_->empty())
    {
        geom->setNormalArray(normal_, osg::Array::BIND_PER_VERTEX);
    }

    if (indices_ && !indices_->empty())
    {
        geom->addPrimitiveSet(indices_);
    }
    else
    {
        // fallback: ·ÇË÷Òý»æÖÆ
        //geom->addPrimitiveSet(new osg::DrawArrays(data.mode_, 0, vert_->size()));
    }

    return geom;
}

MeshData get_mesh_func(Entity e)
{
    auto& drawables = cv_._drawables;
    int idx = (e.index + rand()) % drawables.size();
    auto* geom = drawables[idx]->asGeometry();

    osg::DrawElementsUShort* ps = dynamic_cast<osg::DrawElementsUShort*>(geom->getPrimitiveSet(0));
    auto t = ps->getType();

    auto m = ps->getMode();

    MeshData mesh;
    mesh.vert_ = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    mesh.color_ = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());

    mesh.normal_ = dynamic_cast<osg::Vec3Array *>(geom->getNormalArray());
    mesh.indices_ = new osg::DrawElementsUInt(m);
    //mesh.indices_->setMode(m);

    if (mesh.color_ == nullptr)
    {
   //   std::cout << "null color \n";
    }
  //  std::cout << " color sz:  " << mesh.normal_->size() << "\n";
    int cnt = ps->getNumIndices();
                       
    for (int i = 0; i < cnt; i++)
    {
        int idx = ps->getElement(i);
        mesh.indices_->push_back(idx);
    }


    return mesh;
}
