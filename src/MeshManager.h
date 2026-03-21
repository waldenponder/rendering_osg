#pragma once

#include <vector>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Vec2>

#include "EntityManager.h"

// 网格数据组件，包含渲染所需的几何数据
struct MeshData
{
    // 索引缓冲（三角形索引，可选）
    std::vector<uint32_t> indices;

    // 顶点属性
    std::vector<osg::Vec3> positions;
    std::vector<osg::Vec3> normals;       // 可能为空
    std::vector<osg::Vec4> colors;        // 可能为空
    std::vector<osg::Vec2> texCoords;     // 可能为空

    // 辅助信息
    bool hasNormals() const { return !normals.empty(); }
    bool hasColors() const { return !colors.empty(); }
    bool hasTexCoords() const { return !texCoords.empty(); }
    size_t vertexCount() const { return positions.size(); }
    size_t indexCount() const { return indices.size(); }

    // 清空数据
    void clear() {
        indices.clear();
        positions.clear();
        normals.clear();
        colors.clear();
        texCoords.clear();
    }

    // 检查是否有有效数据
    bool isValid() const {
        return !positions.empty() && (indices.empty() || indices.size() % 3 == 0);
    }
};

class MeshManager
{
public:
    // 初始化容量
    void init(size_t capacity)
    {
        meshes.resize(capacity);
        dirty.resize(capacity, true);
    }

    // 确保索引位置可用
    void ensure(size_t index)
    {
        if (index >= meshes.size())
        {
            size_t newSize = index + 1;
            meshes.resize(newSize);
            dirty.resize(newSize, true);
        }
    }

    // 为实体设置网格数据（移动版本）
    void set_mesh(Entity e, MeshData data)
    {
        uint32_t idx = e.index;
        ensure(idx);
        meshes[idx] = std::move(data);
        dirty[idx] = true;
        // 可在此触发 GPU 更新标记
    }

    // 为实体设置网格数据（拷贝版本）
    void set_mesh(Entity e, const MeshData& data)
    {
        uint32_t idx = e.index;
        ensure(idx);
        meshes[idx] = data;
        dirty[idx] = true;
    }

    // 获取实体的网格数据（只读）
    const MeshData& get_mesh(Entity e) const
    {
        static const MeshData emptyMesh;
        if (e.index >= meshes.size())
            return emptyMesh;
        return meshes[e.index];
    }

    // 获取实体的网格数据（可写）
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

    // 清除实体的网格数据
    void clear_mesh(Entity e)
    {
        if (e.index < meshes.size())
        {
            meshes[e.index].clear();
            dirty[e.index] = true;
        }
    }

    // 标记某个实体的网格数据为脏（例如顶点数据修改后）
    void mark_dirty(Entity e)
    {
        if (e.index < meshes.size())
            dirty[e.index] = true;
    }

    // 批量更新所有脏数据（如上传到 GPU）
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
    std::vector<MeshData> meshes;   // 每个实体对应的网格数据
    std::vector<bool> dirty;        // 标记网格是否需要更新到 GPU
};