#pragma once
// C++11. Entity must have a stable std::less ordering (prefer an ID/handle).
// Single root; automatic initial extent; persistent cell IDs.
// Single-threaded: externally synchronize mutation and queries.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rbrqt {
struct Bounds {
    double minX, minY, minZ, maxX, maxY, maxZ;

    Bounds() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {
    }

    // Compatibility: old XY call sites describe geometry in the Z=0 plane.
    // For 3D geometry always use the six-argument constructor.
    Bounds(double x0, double y0, double x1, double y1)
        : minX(x0), minY(y0), minZ(0), maxX(x1), maxY(y1), maxZ(0) {
    }

    Bounds(double x0, double y0, double z0,
           double x1, double y1, double z1)
        : minX(x0), minY(y0), minZ(z0), maxX(x1), maxY(y1), maxZ(z1) {
    }

    bool valid() const {
        return std::isfinite(minX) && std::isfinite(minY) && std::isfinite(minZ)
               && std::isfinite(maxX) && std::isfinite(maxY) && std::isfinite(maxZ)
               && minX <= maxX && minY <= maxY && minZ <= maxZ;
    }

    double centerX() const { return minX * .5 + maxX * .5; }
    double centerY() const { return minY * .5 + maxY * .5; }

    void expand(const Bounds &b) {
        minX = std::min(minX, b.minX);
        maxX = std::max(maxX, b.maxX);
        minY = std::min(minY, b.minY);
        maxY = std::max(maxY, b.maxY);
        minZ = std::min(minZ, b.minZ);
        maxZ = std::max(maxZ, b.maxZ);
    }

    bool overlaps(const Bounds &b) const {
        return minX <= b.maxX && maxX >= b.minX
               && minY <= b.maxY && maxY >= b.minY
               && minZ <= b.maxZ && maxZ >= b.minZ;
    }
};

struct Ray {
    double origin[3], direction[3];
    double tMin, tMax;

    Ray(double ox, double oy, double oz, double dx, double dy, double dz,
        double nearT = 0,
        double farT = std::numeric_limits<double>::infinity())
        : origin{ox, oy, oz}, direction{dx, dy, dz}, tMin(nearT), tMax(farT) {
    }
};

}

template <class Entity>
class RBRAdaptiveQuadTree {
public:
    using Key = std::uint64_t; // 0 = null. IDs are never reused by this instance.
    using Bounds = rbrqt::Bounds;
    using Ray = rbrqt::Ray;
    using BoundsFn = std::function<Bounds(const Entity &)>;
    using WeightFn = std::function<std::uint64_t(const Entity &)>;

    struct Config {
        std::size_t maxEntities;
        std::uint64_t maxWeight;
        unsigned maxDepth; // Initial root depth budget; expansion adds outer levels.
        double collapseRatio;

        Config() : maxEntities(256), maxWeight(4ULL * 1024 * 1024),
                   maxDepth(16), collapseRatio(.5) {
        }
    };

    struct Region {
        double minX, minY, size; // Nominal XY square; NOT used to prune picks.
        Region() : minX(0), minY(0), size(0) {
        }

        Region(double x, double y, double s) : minX(x), minY(y), size(s) {
        }

        bool contains(const Bounds &b) const {
            return b.minX >= minX && b.minY >= minY
                   && b.maxX <= minX + size && b.maxY <= minY + size;
        }
    };

    struct Cell {
        Key key, parent, children[4];
        Region region;
        std::vector<Entity> entities;
        Bounds bounds; // Actual world-space 3D union, including at internal nodes.
        std::size_t count;
        std::uint64_t weight;
        bool split, hasBounds;

        Cell() : key(0), parent(0), children{0, 0, 0, 0}, count(0), weight(0),
                 split(false), hasBounds(false) {
        }
    };

    RBRAdaptiveQuadTree(const Config &c, BoundsFn b, WeightFn w)
        : config_(c), boundsFn_(b), weightFn_(w), root_(0), nextKey_(1),
          minSize_(0), batchDepth_(0) {
        if (!b || !w || !c.maxEntities || !c.maxWeight || !c.maxDepth
            || c.maxDepth > 48 || !std::isfinite(c.collapseRatio)
            || c.collapseRatio <= 0 || c.collapseRatio >= 1)
            throw std::invalid_argument("invalid quadtree config");
    }

    // Recommended first load: validate all bounds before clearing the old tree.
    // Also use Build deliberately to recalibrate resolution after a huge scene change.
    // Build resets cell membership: old render nodes must be removed via dirty keys.
    void Build(const std::vector<Entity> &entities) {
        requireCommitted();
        std::vector<Prepared> input;
        bool have = false;
        Bounds scene;
        for (std::size_t n = 0; n < entities.size(); ++n) {
            const Bounds b = boundsFn_(entities[n]);
            if (!b.valid())
                throw std::invalid_argument("invalid world bounds");
            const std::uint64_t w = weightFn_(entities[n]);
            input.push_back(Prepared(entities[n], b, w));
            if (!have) {
                scene = b;
                have = true;
            } else
                scene.expand(b);
        }
        Region initial;
        if (have)
            initial = initialRegion(scene); // Validate numeric range before Clear.
        Clear();
        if (!have)
            return;
        initialize(initial);
        BeginBatch();
        for (std::size_t n = 0; n < input.size(); ++n)
            upsert(input[n].entity, input[n].bounds, input[n].weight);
        EndBatch();
    }

    void Clear() {
        requireCommitted();
        for (typename CellMap::const_iterator i = cells_.begin(); i != cells_.end(); ++i)
            if (!i->second.split)
                dirty_.insert(i->first);
        cells_.clear();
        entries_.clear();
        touched_.clear();
        root_ = 0;
        minSize_ = 0;
        // Keep nextKey_ monotonic to distinguish removed cells from new ones.
    }

    void BeginBatch() { ++batchDepth_; }

    void EndBatch() {
        if (!batchDepth_)
            throw std::logic_error("unmatched EndBatch");
        if (--batchDepth_ == 0)
            commit();
    }

    void InsertOrUpdate(const Entity &e) {
        const Bounds b = boundsFn_(e);
        if (!b.valid())
            throw std::invalid_argument("invalid world bounds");
        upsert(e, b, weightFn_(e));
        if (!batchDepth_)
            commit();
    }

    void InsertOrUpdate(const std::vector<Entity> &entities) {
        // Empty-tree batch bootstraps from ALL objects, not just its first object.
        if (!root_ && !batchDepth_) {
            Build(entities);
            return;
        }
        BeginBatch();
        try {
            for (std::size_t n = 0; n < entities.size(); ++n)
                InsertOrUpdate(entities[n]);
        } catch (...) {
            EndBatch();
            throw;
        }
        EndBatch();
    }

    bool Remove(const Entity &e) {
        typename EntryMap::iterator i = entries_.find(e);
        if (i == entries_.end())
            return false;
        detach(i);
        entries_.erase(i);
        if (!batchDepth_)
            commit();
        return true;
    }

    void Remove(const std::vector<Entity> &entities) {
        BeginBatch();
        for (std::size_t n = 0; n < entities.size(); ++n)
            Remove(entities[n]);
        EndBatch();
    }

    bool GetKey(const Entity &e, Key &out) const {
        requireCommitted();
        typename EntryMap::const_iterator i = entries_.find(e);
        if (i == entries_.end())
            return false;
        out = i->second.key;
        return true;
    }

    const Cell *GetCell(Key key) const {
        requireCommitted();
        typename CellMap::const_iterator i = cells_.find(key);
        return i == cells_.end() || i->second.split || !i->second.count ? 0 : &i->second;
    }

    Key RootKey() const {
        requireCommitted();
        return root_;
    }

    double FinestSize() const { return minSize_; }
    std::size_t Size() const { return entries_.size(); }

    std::set<Key> TakeDirtyKeys() {
        requireCommitted();
        std::set<Key> out;
        out.swap(dirty_);
        return out;
    }

    template <class F>
    void ForEachCell(F visit) const {
        requireCommitted();
        for (typename CellMap::const_iterator i = cells_.begin(); i != cells_.end(); ++i)
            if (!i->second.split && i->second.count)
                visit(i->second);
    }

    template <class F>
    void QueryBounds(const Bounds &b, F visit) const {
        requireCommitted();
        if (!b.valid())
            throw std::invalid_argument("invalid query bounds");
        const auto test = [&b](const Bounds &box) { return box.overlaps(b); };
        if (root_)
            query(root_, test, visit);
    }

    template <class F>
    void QueryRay(const Ray &r, F visit) const {
        requireCommitted();
        bool nonzero = false;
        for (int a = 0; a < 3; ++a) {
            if (!std::isfinite(r.origin[a]) || !std::isfinite(r.direction[a]))
                throw std::invalid_argument("invalid ray");
            nonzero = nonzero || r.direction[a] != 0;
        }
        if (!nonzero || !std::isfinite(r.tMin) || std::isnan(r.tMax) || r.tMax < r.tMin)
            throw std::invalid_argument("invalid ray range");
        const auto test = [&r](const Bounds &box) { return rayHits(box, r); };
        if (root_)
            query(root_, test, visit);
    }

private:
    struct Entry {
        Bounds bounds;
        std::uint64_t weight;
        Key key;
        std::size_t slot;

        Entry(const Bounds &b, std::uint64_t w) : bounds(b), weight(w), key(0), slot(0) {
        }
    };

    struct Prepared {
        Entity entity;
        Bounds bounds;
        std::uint64_t weight;

        Prepared(const Entity &e, const Bounds &b, std::uint64_t w) : entity(e),
            bounds(b), weight(w) {
        }
    };

    using EntryMap = std::map<Entity, Entry>;
    using CellMap = std::map<Key, Cell>;
    Config config_;
    BoundsFn boundsFn_;
    WeightFn weightFn_;
    Key root_, nextKey_;
    double minSize_;
    unsigned batchDepth_;
    EntryMap entries_;
    CellMap cells_;
    std::set<Key> touched_, dirty_;

    void requireCommitted() const {
        if (batchDepth_ || !touched_.empty())
            throw std::logic_error("finish batch first");
    }

    static Region initialRegion(const Bounds &b) {
        const double dx = b.maxX - b.minX, dy = b.maxY - b.minY;
        // Relative numeric floor only handles degenerate/very tiny extents.
        const double scale = std::max(std::max(std::abs(b.minX), std::abs(b.maxX)),
                                      std::max(std::abs(b.minY), std::abs(b.maxY)));
        const double numericFloor = std::max(scale, 1.0) * std::numeric_limits<
                                        double>::epsilon() * 64;
        double s = std::max(std::max(dx, dy), numericFloor);
        s = std::nextafter(s, std::numeric_limits<double>::infinity());
        Region r(b.minX, b.minY, s);
        if (!std::isfinite(s) || !std::isfinite(r.minX + s) || !std::isfinite(r.minY + s)
            || r.minX + s <= r.minX || r.minY + s <= r.minY || !r.contains(b))
            throw std::out_of_range("scene extent exceeds numeric range");
        return r;
    }

    Key makeCell(const Region &r, Key parent) {
        if (nextKey_ == std::numeric_limits<Key>::max())
            throw std::overflow_error("cell IDs exhausted");
        const Key id = nextKey_++;
        Cell c;
        c.key = id;
        c.parent = parent;
        c.region = r;
        cells_.emplace(id, std::move(c));
        return id;
    }

    void initialize(const Region &r) {
        minSize_ = std::ldexp(r.size, -static_cast<int>(config_.maxDepth));
        if (minSize_ == 0)
            minSize_ = std::numeric_limits<double>::denorm_min();
        root_ = makeCell(r, 0);
        touched_.insert(root_);
    }

    static int quadrant(const Region &r, const Bounds &b) {
        return (b.centerX() >= r.minX + r.size * .5 ? 1 : 0)
               | (b.centerY() >= r.minY + r.size * .5 ? 2 : 0);
    }

    static Region childRegion(const Region &r, int q) {
        const double s = r.size * .5;
        return Region(r.minX + ((q & 1) ? s : 0), r.minY + ((q & 2) ? s : 0), s);
    }

    void ensureRoot(const Bounds &b) {
        if (!root_) {
            initialize(initialRegion(b));
            return;
        }
        // Plan all expansions first; numeric rejection does not partially expand.
        std::vector<std::pair<Region, int>> plan;
        Region r = cells_.find(root_)->second.region;
        while (!r.contains(b)) {
            const bool left = b.minX < r.minX, down = b.minY < r.minY;
            const double s = r.size * 2;
            Region larger(r.minX - (left ? r.size : 0), r.minY - (down ? r.size : 0), s);
            if (!std::isfinite(s) || !std::isfinite(larger.minX)
                || !std::isfinite(larger.minY) || !std::isfinite(larger.minX + s)
                || !std::isfinite(larger.minY + s) || s <= r.size || plan.size() >= 128)
                throw std::out_of_range("cannot expand root within numeric range");
            plan.push_back(std::make_pair(larger, (left ? 1 : 0) | (down ? 2 : 0)));
            r = larger;
        }
        for (std::size_t n = 0; n < plan.size(); ++n) {
            const Key old = root_, p = makeCell(plan[n].first, 0);
            Cell &parent = cells_.find(p)->second;
            parent.split = true;
            parent.children[plan[n].second] = old;
            cells_.find(old)->second.parent = p;
            root_ = p;
            touched_.insert(p);
            // Existing leaf IDs/members stay unchanged by root expansion itself.
        }
    }

    Key locate(Key id, const Bounds &b) {
        Cell &c = cells_.find(id)->second;
        if (!c.split)
            return id;
        const int q = quadrant(c.region, b);
        if (!c.children[q]) {
            c.children[q] = makeCell(childRegion(c.region, q), id);
            touched_.insert(c.children[q]);
        }
        return locate(c.children[q], b);
    }

    void mark(Key id) {
        while (id) {
            touched_.insert(id);
            id = cells_.find(id)->second.parent;
        }
    }

    void attach(Key id, const Entity &e) {
        Cell &c = cells_.find(id)->second;
        Entry &en = entries_.find(e)->second;
        en.key = id;
        en.slot = c.entities.size();
        c.entities.push_back(e);
        dirty_.insert(id);
        mark(id);
    }

    void detach(typename EntryMap::iterator i) {
        const Entry &en = i->second;
        Cell &c = cells_.find(en.key)->second;
        if (en.slot + 1 != c.entities.size()) {
            c.entities[en.slot] = c.entities.back();
            entries_.find(c.entities[en.slot])->second.slot = en.slot;
        }
        c.entities.pop_back();
        dirty_.insert(en.key);
        mark(en.key);
    }

    void upsert(const Entity &e, const Bounds &b, std::uint64_t w) {
        ensureRoot(b);
        const Key target = locate(root_, b);
        typename EntryMap::iterator i = entries_.find(e);
        if (i != entries_.end()) {
            if (i->second.key == target) {
                i->second.bounds = b;
                i->second.weight = w;
                dirty_.insert(target);
                mark(target);
                return;
            }
            detach(i);
            entries_.erase(i);
        }
        entries_.emplace(e, Entry(b, w));
        attach(target, e);
    }

    static std::uint64_t addWeight(std::uint64_t a, std::uint64_t b) {
        const std::uint64_t cap = std::numeric_limits<std::uint64_t>::max();
        return b > cap - a ? cap : a + b;
    }

    static void reset(Cell &c) {
        c.count = 0;
        c.weight = 0;
        c.hasBounds = false;
    }

    static void accumulate(Cell &c, const Bounds &b, std::size_t n, std::uint64_t w) {
        if (!c.hasBounds) {
            c.bounds = b;
            c.hasBounds = true;
        } else
            c.bounds.expand(b);
        c.count += n;
        c.weight = addWeight(c.weight, w);
    }

    void recalcLeaf(Cell &c) {
        reset(c);
        for (std::size_t n = 0; n < c.entities.size(); ++n) {
            const Entry &en = entries_.find(c.entities[n])->second;
            accumulate(c, en.bounds, 1, en.weight);
        }
    }

    bool canSplit(const Cell &c) const {
        if (c.count < 2 || (c.count <= config_.maxEntities && c.weight <= config_.
                            maxWeight)
            || c.region.size * .5 < minSize_)
            return false;
        const double mx = c.region.minX + c.region.size * .5, my =
                         c.region.minY + c.region.size * .5;
        if (mx <= c.region.minX || mx >= c.region.minX + c.region.size
            || my <= c.region.minY || my >= c.region.minY + c.region.size)
            return false;
        const Bounds &first = entries_.find(c.entities.front())->second.bounds;
        for (std::size_t n = 1; n < c.entities.size(); ++n) {
            const Bounds &b = entries_.find(c.entities[n])->second.bounds;
            if (b.centerX() != first.centerX() || b.centerY() != first.centerY())
                return true;
        }
        return false; // Coincident XY centers cannot be spatially separated.
    }

    bool canCollapse(const Cell &c) const {
        return static_cast<long double>(c.count) <= config_.maxEntities * static_cast<long
                   double>(config_.collapseRatio)
               && static_cast<long double>(c.weight) <= config_.maxWeight * static_cast<
                   long double>(config_.collapseRatio);
    }

    void collect(Key id, std::vector<Entity> &out) const {
        const Cell &c = cells_.find(id)->second;
        if (!c.split)
            out.insert(out.end(), c.entities.begin(), c.entities.end());
        else
            for (int q = 0; q < 4; ++q)
                if (c.children[q])
                    collect(c.children[q], out);
    }

    void eraseSubtree(Key id) {
        const Cell &c = cells_.find(id)->second;
        Key children[4];
        std::copy(c.children, c.children + 4, children);
        for (int q = 0; q < 4; ++q)
            if (children[q])
                eraseSubtree(children[q]);
        dirty_.insert(id);
        touched_.erase(id);
        cells_.erase(id);
    }

    void normalize(Key id, bool force = false) {
        if (!force && !touched_.count(id))
            return;
        Cell &c = cells_.find(id)->second;
        bool newlySplit = false;
        if (!c.split) {
            recalcLeaf(c);
            if (!canSplit(c))
                return;
            std::vector<Entity> moved;
            moved.swap(c.entities);
            c.split = true;
            newlySplit = true;
            dirty_.insert(id);
            for (std::size_t n = 0; n < moved.size(); ++n) {
                const Entry &en = entries_.find(moved[n])->second;
                const int q = quadrant(c.region, en.bounds);
                if (!c.children[q])
                    c.children[q] = makeCell(childRegion(c.region, q), id);
                attach(c.children[q], moved[n]);
            }
        }
        reset(c);
        for (int q = 0; q < 4; ++q)
            if (c.children[q]) {
                const Key child = c.children[q];
                normalize(child, newlySplit);
                const Cell &cc = cells_.find(child)->second;
                if (!cc.count) {
                    eraseSubtree(child);
                    c.children[q] = 0;
                    continue;
                }
                accumulate(c, cc.bounds, cc.count, cc.weight);
            }
        // Preserve a one-child branch (especially newly expanded root wrappers).
        // Collapse multiple occupied children only; unary root pruning below keeps IDs.
        int occupied = 0;
        for (int q = 0; q < 4; ++q)
            if (c.children[q])
                ++occupied;
        if (occupied > 1 && canCollapse(c)) {
            std::vector<Entity> moved;
            collect(id, moved);
            for (int q = 0; q < 4; ++q)
                if (c.children[q]) {
                    eraseSubtree(c.children[q]);
                    c.children[q] = 0;
                }
            c.split = false;
            for (std::size_t n = 0; n < moved.size(); ++n)
                attach(id, moved[n]);
            recalcLeaf(c);
            dirty_.insert(id);
        }
    }

    void commit() {
        if (!root_) {
            touched_.clear();
            return;
        }
        normalize(root_);
        if (!cells_.find(root_)->second.count) {
            eraseSubtree(root_);
            root_ = 0;
            minSize_ = 0;
        } else {
            // Remove empty outer root wrappers without changing the surviving subtree.
            while (cells_.find(root_)->second.split) {
                const Cell &c = cells_.find(root_)->second;
                int occupied = 0;
                Key only = 0;
                for (int q = 0; q < 4; ++q)
                    if (c.children[q]) {
                        ++occupied;
                        only = c.children[q];
                    }
                if (occupied != 1)
                    break;
                const Cell &cc = cells_.find(only)->second;
                // Full bounds containment is needed before shrinking the nominal root.
                if (!cc.region.contains(cc.bounds))
                    break;
                const Key old = root_;
                root_ = only;
                cells_.find(only)->second.parent = 0;
                dirty_.insert(old);
                touched_.erase(old);
                cells_.erase(old);
            }
        }
        touched_.clear();
    }

    template <class Test, class F>
    void query(Key id, const Test &test, F &visit) const {
        const Cell &c = cells_.find(id)->second;
        if (!c.hasBounds || !test(c.bounds))
            return;
        if (c.split) {
            for (int q = 0; q < 4; ++q)
                if (c.children[q])
                    query(c.children[q], test, visit);
        } else
            for (std::size_t n = 0; n < c.entities.size(); ++n) {
                const Entity &e = c.entities[n];
                if (test(entries_.find(e)->second.bounds))
                    visit(e);
            }
    }

    static bool rayHits(const Bounds &b, const Ray &r) {
        const double lo[3] = {b.minX, b.minY, b.minZ}, hi[3] = {b.maxX, b.maxY, b.maxZ};
        long double nearT = r.tMin, farT = r.tMax;
        for (int a = 0; a < 3; ++a) {
            if (r.direction[a] == 0) {
                if (r.origin[a] < lo[a] || r.origin[a] > hi[a])
                    return false;
                continue;
            }
            long double u = (static_cast<long double>(lo[a]) - r.origin[a]) / r.direction[
                                a];
            long double v = (static_cast<long double>(hi[a]) - r.origin[a]) / r.direction[
                                a];
            if (u > v)
                std::swap(u, v);
            nearT = std::max(nearT, u);
            farT = std::min(farT, v);
            if (nearT > farT)
                return false;
        }
        return true;
    }
};

/*
接入说明（C++11，独立头文件，不依赖旧版 RBRAdaptiveCellIndex.h）

using Index = RBRAdaptiveQuadTree<EntityId>;
Index::Config config;
config.maxEntities = 256;
config.maxWeight = 4ULL * 1024 * 1024; // 指标由 WeightFn 决定，不一定是字节。
config.maxDepth = 16;

Index index(config,
    [&](const EntityId& id) -> Index::Bounds {
        // 必须包含当前变换，返回世界坐标包围盒。
        auto b = GetWorldBounds(id);
        return Index::Bounds(b.minX, b.minY, b.minZ, b.maxX, b.maxY, b.maxZ);
    },
    [&](const EntityId& id) -> std::uint64_t {
        return GetVertexCount(id); // 全部对象使用同一种负载指标。
    });

// 首次加载：根据全部对象的 XY 包围盒建立正方形根区域。
index.Build(allEntityIds);

// 增加 / 移动 / 几何或 weight 改变。
index.InsertOrUpdate(id);
index.Remove(id);

// 批量操作：统一重算受影响叶子和祖先；未变化分支不递归扫描。
index.BeginBatch();
for (const auto& id : changedIds) index.InsertOrUpdate(id);
for (const auto& id : deletedIds) index.Remove(id);
index.EndBatch();

// cell ID 改成 uint64_t；不再有 level/x/y 或 keyFor。
Index::Key key;
if (index.GetKey(id, key)) { UseCellID(key); }

// 批量结束后消费 dirty；GetCell 返回空表示旧批次需要删除。
const auto dirty = index.TakeDirtyKeys();
for (auto key : dirty) {
    RemoveOldMergedNodes(key);
    const auto* cell = index.GetCell(key);
    if (cell) RebuildMergedNodes(key, cell->entities);
}

// 只返回实际 AABB 相交的候选，每个对象最多出现一次。
index.QueryRay(Index::Ray(ox, oy, oz, dx, dy, dz), [&](const EntityId& id) {
    ExactIntersectAndCompareNearestHit(id);
});

设计边界：
1. Cell::region 是中心点归属区域；Cell::bounds 是实际 3D 几何并集。
   跨 cell 对象仅归属一个叶节点，查询只使用实际 bounds，不能用 region 剪枝。
2. maxDepth 决定首次根尺寸 / 2^maxDepth 的最细尺寸。扩根新增外层，
   不放大最细尺寸，所以扩根后的实际总层数可以超过 maxDepth。
   数值精度不足、最细尺寸、中心重合均会终止拆分。预算是软上限。
3. 原有子树在扩根时成为新根的一个子树，ID 不重新编码。
   正常拆分、收缩、对象迁移仍可能改变成员所属 ID，并发出 dirty。
4. 删除后清理空分支；多个低负载子树可合为父叶子。
   单子分支保留，最外层单子根在完整 bounds 可容纳时向内收缩。
   这样避免为了收缩单子路径而无意义地重建合并几何。
5. 首次建议 Build(allIds)。在空树中逐个插入，会以首个对象初始化最细尺寸；
   InsertOrUpdate(vector) 在非 batch 的空树中也会自动按全体对象 Build。
   场景尺度根本改变时显式 Build 重建，可重新校准最细尺寸。
6. cell 内合并仍需按材质、状态、图元类型等兼容条件分组。
   保存合并 primitive/index 区间到 entity ID 的映射用于精确 pick。
7. Ray 是无宽度射线；屏幕像素容差拾取需要保守拾取体积粗筛和像素距离精筛。
   对多层建筑，XY 分组效率仍受 Z 重叠影响；本实现不是八叉树。
8. Key 在此实例生命周期内单调递增（包括 Clear/Build），没有跨进程持久性。
   cell 指针不要跨 mutation 保存。ID/handle 的比较顺序必须稳定。
9. 查询回调不可修改此树；batch 期间禁止查询、取 key/cell 或消费 dirty。
   不提供内部线程同步、异步 GPU 重建和分配失败时的事务回滚。
   如批次重建失败，调用方须保留未处理 dirty 并重试。
10. 查询为粗筛，不会自动获取最近精确命中；包围盒高度重叠仍会影响性能。
*/