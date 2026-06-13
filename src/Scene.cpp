// pch.cpp: 与预编译标头对应的源文件；编译成功所必需的
#include "Scene.h"

#include "pch.h" 


#include "manager/EntityManager.h"
#include "manager/MeshManager.h"
#include "manager/RenderInfoManager.h"
#include "manager/TransformManager.h"

EntityManager entity_manager;
TransformManager transform_manager;
RenderInfoManager render_info_manager;
MeshManager mesh_manager;

Scene::Scene()
{
    transform_manager.set_entity_manager(&entity_manager);
    render_info_manager.set_entity_manager(&entity_manager);
}

Scene& Scene::instance() 
{
    static Scene s;
    return s;
}

void Scene::init(size_t capacity)
{
    entity_manager = EntityManager();
    transform_manager.init(capacity);
    render_info_manager.init(capacity);
    mesh_manager.init(capacity);
}

void Scene::ensure(size_t index)
{
    transform_manager.ensure(index);
    render_info_manager.ensure(index);
    mesh_manager.ensure(index);
}

void Scene::dirty(Entity e)
{
    transform_manager.mark_dirty(e);
    render_info_manager.mark_dirty(e);
}
