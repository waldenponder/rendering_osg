#pragma once

struct Entity;
class EntityManager;
class TransformManager;
class RenderInfoManager;
class MeshManager;

extern EntityManager entity_manager;
extern TransformManager transform_manager;
extern RenderInfoManager render_info_manager;
extern MeshManager mesh_manager;

class Scene {
public:
  Scene();

    static Scene &instance();

    void init(size_t capacity);
    void ensure(size_t index);

    void dirty(Entity e);
    
};
