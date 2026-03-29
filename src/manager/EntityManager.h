#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

#include "../Scene.h"

static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

struct Entity
{
    uint32_t index;
    uint32_t version;
    //  std::string debug_name;
};

class EntityManager
{
public:
    EntityManager() = default;

    void init(size_t capacity)
    {
        versions.reserve(capacity);
    }

    Entity create()
    {
        uint32_t index;

        if (!freeList.empty())
        {
            index = freeList.back();
            freeList.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(versions.size());
            versions.push_back(0);
            nodes.emplace_back();

            if (nodes.size() <= index)
            {
                std::cout << "\n";
            }
            Scene::instance().ensure(index);
        }

        return {index, versions[index]};
    }

    void destroy(Entity e)
    {
        if (!is_alive(e))
            return;

        uint32_t idx = e.index;

        detach(idx);

        while (nodes[idx].firstChild != INVALID_ID)
        {
            uint32_t child = nodes[idx].firstChild;
            destroy({child, versions[child]});
        }

        versions[idx]++;

        freeList.push_back(idx);
    }

    bool is_alive(Entity e) const
    {
        if (e.index >= versions.size())
            return false;
        return versions[e.index] == e.version;
    }

    void set_parent(Entity child, Entity parent)
    {
        if (!is_alive(child))
            return;

        uint32_t c = child.index;
        uint32_t p = parent.index;

        if (c == p)
            return;

        if (p != INVALID_ID && !is_alive(parent))
            return;

        detach(c);

        if (p == INVALID_ID)
        {
            nodes[c].parent = INVALID_ID;

            Scene::instance().dirty(child);
            return;
        }

        Node& parentNode = nodes[p];
        Node& childNode = nodes[c];

        childNode.parent = p;

        childNode.nextSibling = parentNode.firstChild;
        childNode.prevSibling = INVALID_ID;

        if (parentNode.firstChild != INVALID_ID)
        {
            nodes[parentNode.firstChild].prevSibling = c;
        }

        parentNode.firstChild = c;
        Scene::instance().dirty(child);
    }

    Entity get_parent(Entity e) const
    {
        if (!is_alive(e))
            return invalid_entity();

        uint32_t p = nodes[e.index].parent;
        if (p == INVALID_ID)
            return invalid_entity();

        return {p, versions[p]};
    }

    uint32_t get_parent_index(uint32_t idx) const { return nodes[idx].parent; }

    template <typename Func>
    void for_each_child(Entity e, Func func)
    {
        if (!is_alive(e))
            return;

        uint32_t child = nodes[e.index].firstChild;

        while (child != INVALID_ID)
        {
            func(Entity{child, versions[child]});
            child = nodes[child].nextSibling;
        }
    }


    template <typename Func>
    void for_each_child_index(uint32_t idx, Func func)
    {
        uint32_t child = nodes[idx].firstChild;

        while (child != INVALID_ID)
        {
            func(child);
            child = nodes[child].nextSibling;
        }
    }

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = default;
    EntityManager& operator=(EntityManager&&) = default;

private:
    struct Node
    {
        uint32_t parent = INVALID_ID;
        uint32_t firstChild = INVALID_ID;
        uint32_t nextSibling = INVALID_ID;
        uint32_t prevSibling = INVALID_ID;
    };

    void detach(uint32_t idx)
    {
        Node& n = nodes[idx];

        if (n.parent == INVALID_ID)
            return;

        Node& parent = nodes[n.parent];

        if (parent.firstChild == idx)
        {
            parent.firstChild = n.nextSibling;
        }

        if (n.prevSibling != INVALID_ID)
        {
            nodes[n.prevSibling].nextSibling = n.nextSibling;
        }

        if (n.nextSibling != INVALID_ID)
        {
            nodes[n.nextSibling].prevSibling = n.prevSibling;
        }

        n.parent = INVALID_ID;
        n.nextSibling = INVALID_ID;
        n.prevSibling = INVALID_ID;
    }

    bool is_ancestor(uint32_t child, uint32_t parent)
    {
        uint32_t p = nodes[child].parent;
        // int cnt = 0;
        while (p != INVALID_ID)
        {
            // std::cout << p << "   cnt: " << cnt++ << std::endl;
            if (p == parent)
                return true;
            p = nodes[p].parent;
        }
        return false;
    }

    static Entity invalid_entity() { return {INVALID_ID, 0}; }

private:
    std::vector<uint32_t> versions;
    std::vector<uint32_t> freeList;
    std::vector<Node> nodes; // hierarchy
};
