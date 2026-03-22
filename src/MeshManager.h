#pragma once

#include <vector>
#include <osg/Array>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Vec2>

#include "CollectDrawableVisitor.h"
#include "EntityManager.h"

struct MeshData
{
    osg::ref_ptr<osg::Vec3Array> vert_;
    osg::ref_ptr<osg::Vec3Array> normal_;
    osg::ref_ptr<osg::Vec4Array> color_;
    osg::ref_ptr<osg::DrawElementsUInt> indices_;

    osg::PrimitiveSet::Mode mode_ = osg::PrimitiveSet::TRIANGLES;

    bool dirty_ = true;

    bool isValid() const { return vert_ && !vert_->empty(); }

    void clear()
    {
        if (vert_)
            vert_->clear();
        if (normal_)
            normal_->clear();
        if (color_)
            color_->clear();
        if (indices_)
            indices_->clear();
        dirty_ = true;
    }

    void ensure()
    {
        if (!vert_)
            vert_ = new osg::Vec3Array;
        if (!normal_)
            normal_ = new osg::Vec3Array;
        if (!color_)
            color_ = new osg::Vec4Array;
        if (!indices_)
            indices_ = new osg::DrawElementsUInt(mode_);
    }

    void addVertex(const osg::Vec3& v,
                   const osg::Vec4& c = osg::Vec4(1, 1, 1, 1))
    {
        ensure();
        vert_->push_back(v);
        color_->push_back(c);
        dirty_ = true;
    }

    void addTriangle(uint32_t i0, uint32_t i1, uint32_t i2)
    {
        ensure();
        if (!indices_)
            indices_ = new osg::DrawElementsUInt(mode_);
        indices_->push_back(i0);
        indices_->push_back(i1);
        indices_->push_back(i2);
        dirty_ = true;
    }
};


class MeshManager
{
public:
    void init(size_t capacity)
    {
        meshes.resize(capacity);
        dirty.resize(capacity, true);
    }

    void ensure(size_t index)
    {
        if (index >= meshes.size())
        {
            size_t newSize = index + 1;
            meshes.resize(newSize);
            dirty.resize(newSize, true);
        }
    }

    void set_mesh(Entity e, const MeshData& data)
    {
        uint32_t idx = e.index;
        ensure(idx);
        meshes[idx] = data;
        dirty[idx] = true;
    }

    const MeshData& get_mesh(Entity e) const
    {
        static const MeshData emptyMesh;
        if (e.index >= meshes.size())
            return emptyMesh;
        return meshes[e.index];
    }

    MeshData& get_mesh_mutable(Entity e)
    {
        ensure(e.index);
        dirty[e.index] = true;
        return meshes[e.index];
    }

    // 检查实体是否有有效网格
    bool has_mesh(Entity e) const
    {
        return e.index < meshes.size() && meshes[e.index].isValid();
    }

    void clear_mesh(Entity e)
    {
        if (e.index < meshes.size())
        {
            meshes[e.index].clear();
            dirty[e.index] = true;
        }
    }

    void mark_dirty(Entity e)
    {
        if (e.index < meshes.size())
            dirty[e.index] = true;
    }

    void update_all()
    {
        // 实际场景中，这里会遍历脏标记，将网格数据上传到 GPU 缓冲区
        // 此处仅重置脏标记
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            if (dirty[i] && meshes[i].isValid())
            {
                // 执行上传逻辑...
                dirty[i] = false;
            }
        }
    }

private:
    std::vector<MeshData> meshes; // 每个实体对应的网格数据
    std::vector<bool> dirty; // 标记网格是否需要更新到 GPU
};

osg::ref_ptr<osg::Geometry> createGeometry(MeshData& data);

extern CollectDrawableVisitor cv_;
MeshData get_mesh_func(Entity e);
