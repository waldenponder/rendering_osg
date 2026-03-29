#pragma once
#include <cstdint>
#include <vector>
#include "EntityManager.h"

enum RenderInfoMask : uint32_t
{
    MASK_NONE = 0,
    MASK_LOCAL_VISIBLE = 1 << 0,
    MASK_LAYER = 1 << 1,
    MASK_CATEGORY = 1 << 2,
    MASK_TRANSPARENCY = 1 << 3,
    MASK_LINE_WIDTH = 1 << 4,
    MASK_POLYGON_OFFSET_FACTOR = 1 << 5,
    MASK_POLYGON_OFFSET_UNIT = 1 << 6,
    MASK_HIGHLIGHT = 1 << 7,
};

struct RenderInfo
{
    RenderInfo()
        : visible(1),
          localVisible(1),
          layer(0),
          category(0),
          transparency(0),
          lineWidth(1),
          polygonOffsetFactor(0),
          polygonOffsetUnit(0),
          highlight(0),
          padding(0)
    {
    }

    uint32_t visible : 1; // 最终可见性（路径全可见）
    uint32_t localVisible : 1; // 局部可见性（用户设置）
    int layer : 8;
    int category : 8;
    int transparency : 1;
    int lineWidth : 3;
    int polygonOffsetFactor : 3;
    int polygonOffsetUnit : 3;
    uint32_t highlight : 1;
    int padding : 3;
};

class RenderInfoManager
{
public:
    void init(size_t capacity)
    {
        localInfo.reserve(capacity);
        worldInfo.reserve(capacity);
        dirty.reserve(capacity);
    }

    void ensure(size_t index)
    {
        if (index >= localInfo.size())
        {
            size_t oldSize = localInfo.size();
            size_t newSize = index + 1;
            localInfo.resize(newSize);
            worldInfo.resize(newSize);
            dirty.resize(newSize, true);
            // 新元素已通过 RenderInfo 默认构造函数初始化（localVisible=1，其他=0）
        }
    }

    void set_local_visible(Entity e, bool visible)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].localVisible = visible ? 1 : 0;
        mask[idx] |= MASK_LOCAL_VISIBLE;
        // mark_dirty_recursive(idx);
    }

    void set_local_layer(Entity e, int layer)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].layer = layer;
        mask[idx] |= MASK_LAYER;
        //  mark_dirty_recursive(idx);
    }

    void set_local_category(Entity e, int category)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].category = category;
        mask[idx] |= MASK_CATEGORY;
        // mark_dirty_recursive(idx);
    }

    void set_local_transparency(Entity e, bool transparency)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].transparency = transparency ? 1 : 0;
        mask[idx] |= MASK_TRANSPARENCY;
        //mark_dirty_recursive(idx);
    }

    void set_local_lineWidth(Entity e, int width)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].lineWidth = width;
        mask[idx] |= MASK_LINE_WIDTH;
        //mark_dirty_recursive(idx);
    }

    void set_local_polygonOffsetFactor(Entity e, int factor)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].polygonOffsetFactor = factor;
        mask[idx] |= MASK_POLYGON_OFFSET_FACTOR;
        //mark_dirty_recursive(idx);
    }

    void set_local_polygonOffsetUnit(Entity e, int unit)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].polygonOffsetUnit = unit;
        mask[idx] |= MASK_POLYGON_OFFSET_UNIT;
        //mark_dirty_recursive(idx);
    }

    void set_local_highlight(Entity e, bool highlight)
    {
        uint32_t idx = e.index;
        ensure(idx);
        localInfo[idx].highlight = highlight ? 1 : 0;
        mask[idx] |= MASK_HIGHLIGHT;
        // mark_dirty_recursive(idx);
    }

    // ---------- 获取世界属性 ----------
    const RenderInfo& get_world(Entity e)
    {
        uint32_t idx = e.index;
        ensure(idx);
        update_world(idx);
        return worldInfo[idx];
    }

    // 强制更新所有脏节点
    void update_all()
    {
        for (uint32_t i = 0; i < localInfo.size(); ++i)
        {
            if (dirty[i])
                update_world(i);
        }
    }

    void set_entity_manager(EntityManager* manager) { em = manager; }

    // 递归标记当前节点及所有子孙节点为脏
    void mark_dirty_recursive(uint32_t idx)
    {
        if (dirty[idx])
            return;
        dirty[idx] = true;
        if (em)
        {
            em->for_each_child_index(
                idx, [this](uint32_t child) { mark_dirty_recursive(child); });
        }
    }

private:
    // 计算世界属性（先确保父节点最新，再合并）
    void update_world(uint32_t idx)
    {
        if (!dirty[idx])
            return;

        // 确保父节点是最新的
        uint32_t parent = (em ? em->get_parent_index(idx) : INVALID_ID);
        if (parent != INVALID_ID)
        {
            update_world(parent);
        }

        RenderInfo& world = worldInfo[idx];
        const RenderInfo& local = localInfo[idx];
        const RenderInfo* parentWorld =
            (parent != INVALID_ID) ? &worldInfo[parent] : nullptr;

        // 可见性：自身局部可见 且 父节点世界可见（无父节点时视为可见）
        world.visible =
            local.localVisible && (parentWorld ? parentWorld->visible : 1);

        // 其他属性：若局部显式设置则用局部值，否则继承父节点（或默认值）
#define INHERIT_OR_DEFAULT(f, field, default_val)                              \
  if (mask[idx] & MASK_##field)                                                \
    world.f = local.f;                                                         \
  else if (parentWorld)                                                        \
    world.f = parentWorld->f;                                                  \
  else                                                                         \
    world.f = default_val

        INHERIT_OR_DEFAULT(layer, LAYER, 0);
        INHERIT_OR_DEFAULT(category, CATEGORY, 0);
        INHERIT_OR_DEFAULT(transparency, TRANSPARENCY, 0);
        INHERIT_OR_DEFAULT(lineWidth, LINE_WIDTH, 0);
        INHERIT_OR_DEFAULT(polygonOffsetFactor, POLYGON_OFFSET_FACTOR, 0);
        INHERIT_OR_DEFAULT(polygonOffsetUnit, POLYGON_OFFSET_UNIT, 0);
        INHERIT_OR_DEFAULT(highlight, HIGHLIGHT, 0);
#undef INHERIT_OR_DEFAULT

        dirty[idx] = false;
    }

    std::vector<RenderInfo> localInfo;
    std::vector<RenderInfo> worldInfo;
    std::vector<uint32_t> mask; // 标记哪些属性在局部被显式设置
    std::vector<bool> dirty;
    EntityManager* em = nullptr;
};
