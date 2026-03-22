#pragma once
#include <vector>
#include "manager/EntityManager.h"
#include "manager/MeshManager.h"

struct Batch
{
    std::vector<Entity> entities;
};

class BatchSystem
{
public:
    static std::vector<Batch> computeBatch(const std::vector<Entity>& entities);
    static void applyMatrix(const std::vector<Entity>& entities);
    static std::vector<MeshData> merge(const std::vector<Batch>& batch);
};
