// pch.cpp: 与预编译标头对应的源文件；编译成功所必需的

#include <chrono>
#include <iostream>
#include <random>
#include "manager/EntityManager.h"

// 一般情况下，忽略此文件，但如果你使用的是预编译标头，请保留它。
using namespace std;

//void main()
//{
//    auto t1 = clock();
//
//    Scene::instance().init(50000);
//    auto t2 = clock();
//    cout << "aaa: " << double(t2 - t1) << endl;
//    getchar();
//}


// ==================== 引入你提供的 EntityManager ====================
// 注意：需要将 EntityManager 的完整代码放在这里（略，假设已包含）
// 此处为演示，我会复制你给出的代码并稍作调整（去掉对 Scene
// 的依赖，因为已在上面定义）

// 为测试简洁，我将你的代码直接粘贴过来，并注释掉 create() 中的
// Scene::instance().ensure(index); 在实际测试中，我们保留 Scene
// 调用（上面已定义），不影响性能。 为避免重复，我将省略代码粘贴，但假设
// EntityManager 已定义。 下面的测试代码假设 EntityManager 已存在。

// ==================== 性能测试函数 ====================

#if 0
// 测试1：创建大量实体（顺序）
void testCreatePerformance(EntityManager& em, size_t count)
{
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i)
    {
        em.create();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Create " << count << " entities: " << duration.count()
        << " ms (avg " << duration.count() * 1000.0 / count << " us)"
        << std::endl;
}

// 测试2：销毁所有实体（顺序）
void testDestroyAllPerformance(EntityManager& em,
                               std::vector<Entity>& entities)
{
    auto start = std::chrono::high_resolution_clock::now();
    for (auto& e : entities)
    {
        em.destroy(e);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Destroy " << entities.size()
        << " entities: " << duration.count() << " ms (avg "
        << duration.count() * 1000.0 / entities.size() << " us)"
        << std::endl;
}

// 测试3：构建深度为 depth 的树，每个节点一个孩子（链状）
void testBuildChain(EntityManager& em, size_t depth)
{
    std::vector<Entity> chain;
    for (size_t i = 0; i < depth; ++i)
    {
        chain.push_back(em.create());
        if (i > 0)
        {
            em.set_parent(chain.back(), chain[i - 1]);
        }
    }
    // 验证深度
    std::cout << "Built chain of " << depth << " nodes." << std::endl;
}

// 测试4：构建广度树（每个节点有 branchFactor 个孩子，共 level 层）
void testBuildTree(EntityManager& em, size_t level, size_t branchFactor)
{
    std::vector<Entity> currentLevel;
    // 根节点
    Entity root = em.create();
    currentLevel.push_back(root);
    for (size_t l = 1; l < level; ++l)
    {
        std::vector<Entity> nextLevel;
        for (auto& parent : currentLevel)
        {
            for (size_t b = 0; b < branchFactor; ++b)
            {
                Entity child = em.create();
                em.set_parent(child, parent);
                nextLevel.push_back(child);
            }
        }
        currentLevel.swap(nextLevel);
    }
    size_t totalNodes = (std::pow(branchFactor, level) - 1) / (branchFactor - 1);
    std::cout << "Built tree with " << totalNodes << " nodes (level=" << level
        << ", branch=" << branchFactor << ")" << std::endl;
}

// 测试5：遍历所有节点的子节点（使用 for_each_child）
void testTraversePerformance(EntityManager& em,
                             const std::vector<Entity>& entities)
{
    auto start = std::chrono::high_resolution_clock::now();
    size_t count = 0;
    for (auto& e : entities)
    {
        em.for_each_child(e, [&](Entity child) { ++count; });
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Traverse " << entities.size()
        << " nodes (total children visited " << count
        << "): " << duration.count() << " ms" << std::endl;
}

// 测试6：随机访问设置父子关系
void testRandomSetParent(EntityManager& em, std::vector<Entity>& entities,
                         size_t iterations)
{
  cout << "Randomly set_parent " << iterations << " times..." << std::endl;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, entities.size() - 1);
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i)
    {
        size_t c = dist(gen);
        size_t p = dist(gen);
        if (c != p)
        {
            em.set_parent(entities[c], entities[p]);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Random set_parent " << iterations
        << " times: " << duration.count() << " ms (avg "
        << duration.count() * 1000.0 / iterations << " us)" << std::endl;
}
             
int main2()
{
    EntityManager em;

    const size_t NUM_ENTITIES = 100000; // 10万
    const size_t CHAIN_DEPTH = 20; // 1万链
    const size_t TREE_LEVEL = 6; // 6层
    const size_t BRANCH = 4; // 每个节点4子

    std::cout << "========== EntityManager Performance Test =========="
        << std::endl;

    // 1. 创建大量实体
    testCreatePerformance(em, NUM_ENTITIES);
    // 记录所有实体用于后续测试（销毁时要用）
    // 注意：创建后，需要把实体存起来，因为 create() 会返回 Entity
    // 但上面 testCreatePerformance 没有返回列表，我们重新创建并存储
    std::vector<Entity> allEntities;
    allEntities.reserve(NUM_ENTITIES);
    for (size_t i = 0; i < NUM_ENTITIES; ++i)
    {
        allEntities.push_back(em.create());
    }

    // 2. 设置一些父子关系，构建树形结构
    std::cout << "\nBuilding tree structure..." << std::endl;
    // 构建一个随机树：每个节点随机选择一个已存在的父节点（形成多叉树，避免循环）
    // 注意：确保不会形成循环，这里简单顺序处理：每个节点只选择索引比它小的节点作为父节点
    for (size_t i = 1; i < 2000; ++i)
    {
        uint32_t parentIdx = (i - 1) % i; // 简单的父节点选择，保证无环
        em.set_parent(allEntities[i], allEntities[parentIdx]);
    }
    std::cout << "Built random tree with " << allEntities.size() << " nodes."
        << std::endl;

    // 3. 遍历所有节点的子节点
    testTraversePerformance(em, allEntities);

    // 4. 随机修改父子关系
    testRandomSetParent(em, allEntities, 10000);

    // 5. 构建深度链，测试深层遍历
    EntityManager em2;
    testBuildChain(em2, CHAIN_DEPTH);
    // 取出链上的所有实体（由于 testBuildChain 内部创建并存储，我们需要额外记录）
    // 这里为了简洁，不重复遍历，因为上面已测试过遍历。

    // 6. 构建广度树，测试大量节点管理
    EntityManager em3;
    testBuildTree(em3, TREE_LEVEL, BRANCH);

    // 7. 销毁所有实体
    testDestroyAllPerformance(em, allEntities);

    return 0;
}
#endif
