#include "BatchSystem.h"
#include "manager/TransformManager.h"

std::vector<Batch> BatchSystem::computeBatch(const std::vector<Entity>& entities)
{
    std::vector<Batch> res;

    Batch batch;
    int cnt = 0;
    for (auto& e : entities)
    {
        if (cnt == 1)
        {
            res.push_back(batch);
            batch.entities = {};
            cnt = 0;
        }
        cnt++;
        batch.entities.push_back(e);
    }
    res.push_back(batch);
    return res;
}

void BatchSystem::applyMatrix(const std::vector<Entity>& entities)
{
    for (auto& e : entities)
    {
        auto mat = transform_manager.get_local(e);
        osg::Vec3d pos = mat.getTrans();
        // std::cout << "pos: " <<  pos.x() << "   , " << pos.y() << "  , " << pos.z () << "\n";
        MeshData& data = mesh_manager.get_mesh_mutable(e);
       // osg::ref_ptr<osg::Vec3Array> vv = new osg::Vec3Array;
        for (auto& v : data.vert_->asVector())
        {
            v = v * mat;
            //vv->push_back(tmp);
             //v = v + pos;
        }
        //data.vert_ = vv;
    }
 }

std::vector<MeshData> BatchSystem::merge(const std::vector<Batch>& batchs)
{
    std::vector<MeshData> res;
    res.reserve(batchs.size());

    for (auto& batch : batchs)
    {
        MeshData data;
        data.ensure();

        size_t totalVert = 0;
        size_t totalIdx = 0;

        for (auto e : batch.entities)
        {
            const MeshData& tmp = mesh_manager.get_mesh(e);
            if (!tmp.isValid())
                continue;

            totalVert += tmp.vert_->size();
            if (tmp.indices_)
                totalIdx += tmp.indices_->size();
        }

        data.vert_->reserve(totalVert);
        data.normal_->reserve(totalVert);
        data.color_->reserve(totalVert);
        data.indices_->reserve(totalIdx);

        uint32_t base = 0;

        for (auto e : batch.entities)
        {
            const MeshData& tmp = mesh_manager.get_mesh(e);
            if (!tmp.isValid())
                continue;

            if (tmp.mode_ != data.mode_)
            {
                std::cout << "Warning: Primitive mode mismatch, skipping entity "
                    << e.index << "   " << tmp.mode_ << "   " << data.mode_ << "\n";
                continue;
            }

            //osg::Matrix world = transform_manager.get_world_matrix(e);
            //osg::Matrix normalMat = osg::Matrix::inverse(world);
            //normalMat.transpose(normalMat);

            size_t vertCount = tmp.vert_->size();

            for (auto& v : *tmp.vert_)
            {
                data.vert_->push_back(v);
            }

            if (tmp.normal_ && tmp.normal_->size() == vertCount)
            {
                for (auto& n : *tmp.normal_)
                {
                    //osg::Vec3 nn = osg::Matrix::transform3x3(n, normalMat);
                    //nn.normalize();
                    data.normal_->push_back(n);
                }
            }
            else
            {
                for (size_t i = 0; i < vertCount; ++i)
                    data.normal_->push_back(osg::Vec3(0, 0, 1));
            }

            if (tmp.color_ && tmp.color_->size() == vertCount)
            {
                data.color_->insert(data.color_->end(), tmp.color_->begin(),
                                    tmp.color_->end());
            }
            else
            {
                for (size_t i = 0; i < vertCount; ++i)
                    data.color_->push_back(osg::Vec4(1, 1, 1, 1));
            }


            if (tmp.indices_)
            {
                for (auto idx : *tmp.indices_)
                {
                    data.indices_->push_back(idx + base);
                }
            }
            else
            {
                for (uint32_t i = 0; i < vertCount; ++i)
                {
                    data.indices_->push_back(base + i);
                }
            }

            base += vertCount;
        }

        if (data.isValid())
            res.push_back(data);
    }

    return res;
}
