#pragma once
#include <cstdint>
#include <vector>
#include <osg/Matrix>

#include "EntityManager.h"

class TransformManager
{
public:
    void init(size_t capacity)
    {
        local.resize(capacity, osg::Matrix::identity());
        world.resize(capacity, osg::Matrix::identity());
        dirty.resize(capacity, true);
    }

    void ensure(size_t index)
    {
        if (index >= local.size())
        {
            size_t newSize = index + 1;
            local.resize(newSize, osg::Matrix::identity());
            world.resize(newSize, osg::Matrix::identity());
            dirty.resize(newSize, true);
        }
    }

    void set_local(Entity e, const osg::Matrix& m)
    {
        uint32_t idx = e.index;
        ensure(idx);

        local[idx] = m;

        mark_dirty_recursive(idx);
    }

    const osg::Matrix& get_world(Entity e)
    {
        uint32_t idx = e.index;
            update_world(idx);
        return world[idx];
    }

    void update_world(uint32_t idx)
    {
      if (!dirty[idx])
        return;

        uint32_t parent = em->get_parent_index(idx);

        if (parent == INVALID)
        {
            world[idx] = local[idx];
        }
        else
        {
            update_world(parent);
            world[idx] = world[parent] * local[idx];
        }

        dirty[idx] = false;
    }

    void update_all()
    {
        for (uint32_t i = 0; i < local.size(); i++)
        {
            if (dirty[i])
            {
                update_world(i);
            }
        }
    }

    void set_entity_manager(EntityManager* manager)
    {
        em = manager;
    }

private:
    void mark_dirty_recursive(uint32_t idx)
    {
        if (dirty[idx]) return;

        dirty[idx] = true;

        em->for_each_child_index(idx, [&](uint32_t child)
        {
            mark_dirty_recursive(child);
        });
    }

    static osg::Matrix identity_matrix()
    {
        static osg::Matrix identity = osg::Matrix::identity();
        return identity;
    }

private:
    std::vector<osg::Matrix> local;
    std::vector<osg::Matrix> world;
    std::vector<bool> dirty;

    EntityManager* em = nullptr;
};
