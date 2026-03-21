#pragma once
#include <cstdint>
#include <vector>
static constexpr uint32_t INVALID = 0xFFFFFFFF;

struct Entity
{
    uint32_t index;
    uint32_t version;
};


class EntityManager
{
public:
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
            index = versions.size();
            versions.push_back(0);
            nodes.emplace_back();
        }

        return {index, versions[index]};
    }

    void destroy(Entity e)
    {
        if (!is_alive(e))
            return;

        uint32_t idx = e.index;

        detach(idx);

        while (nodes[idx].firstChild != INVALID)
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
        if (!is_alive(child) || !is_alive(parent))
            return;

        uint32_t c = child.index;
        uint32_t p = parent.index;

        if (c == p)
            return;

        if (is_ancestor(c, p))
            return;

        detach(c);

        Node& parentNode = nodes[p];
        Node& childNode = nodes[c];

        childNode.parent = p;

        childNode.nextSibling = parentNode.firstChild;

        if (parentNode.firstChild != INVALID)
        {
            nodes[parentNode.firstChild].prevSibling = c;
        }

        parentNode.firstChild = c;
    }

    Entity get_parent(Entity e) const
    {
        if (!is_alive(e))
            return invalid_entity();

        uint32_t p = nodes[e.index].parent;
        if (p == INVALID)
            return invalid_entity();

        return {p, versions[p]};
    }

    template <typename Func>
    void for_each_child(Entity e, Func func)
    {
        if (!is_alive(e))
            return;

        uint32_t child = nodes[e.index].firstChild;

        while (child != INVALID)
        {
            func(Entity{child, versions[child]});
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
        uint32_t parent = INVALID;
        uint32_t firstChild = INVALID;
        uint32_t nextSibling = INVALID;
        uint32_t prevSibling = INVALID;
    };

    void detach(uint32_t idx)
    {
        Node& n = nodes[idx];

        if (n.parent == INVALID)
            return;

        Node& parent = nodes[n.parent];

        if (parent.firstChild == idx)
        {
            parent.firstChild = n.nextSibling;
        }

        if (n.prevSibling != INVALID)
        {
            nodes[n.prevSibling].nextSibling = n.nextSibling;
        }

        if (n.nextSibling != INVALID)
        {
            nodes[n.nextSibling].prevSibling = n.prevSibling;
        }

        n.parent = INVALID;
        n.nextSibling = INVALID;
        n.prevSibling = INVALID;
    }

    bool is_ancestor(uint32_t child, uint32_t parent)
    {
        uint32_t p = nodes[child].parent;

        while (p != INVALID)
        {
            if (p == parent)
                return true;
            p = nodes[p].parent;
        }
        return false;
    }

    static Entity invalid_entity() { return {INVALID, 0}; }

private:
    std::vector<uint32_t> versions;
    std::vector<uint32_t> freeList;
    std::vector<Node> nodes; // hierarchy
};
