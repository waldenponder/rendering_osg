#include "test_ecs2.h"

#include <ostream>
#include "manager/EntityManager.h"
#include "manager/MeshManager.h"
#include "manager/RenderInfoManager.h"
#include "manager/TransformManager.h"
#include <cassert>
#include <iostream>
#include <osg/Matrix>
#include <osg/Vec3>

void test_transform()
{
    auto t1 = clock();

    entity_manager.init(1e5);
    transform_manager.init(1e5);
    render_info_manager.init(1e5);
    mesh_manager.init(1e5);

    Entity root = entity_manager.create();
    // root.debug_name = "root";

    std::vector<Entity> entities;

    for (int i = 0; i < 1e5; i++)
    {
        Entity c1 = entity_manager.create();
        //  c1.debug_name = "c1";

        Entity c2 = entity_manager.create();
        // c2.debug_name = "c2";

        entities.emplace_back(c1);
        entities.emplace_back(c2);

        entity_manager.set_parent(c1, root);
        entity_manager.set_parent(c2, c1);

        osg::Matrix m1 = osg::Matrix::translate(osg::Vec3f(11, 22, 0));

        transform_manager.set_local(c1, osg::Matrix::translate(osg::Vec3f(11, 22, 0)));
    }

    auto t2 = clock();

    std::cout << "test_transform: " << (t2 - t1) << "\n";

    std::vector<osg::Matrixd> mats;
    mats.reserve(entities.size());

    auto t3 = clock();

    osg::Matrix m1 = osg::Matrix::translate(osg::Vec3f(0, 0, 0));
    transform_manager.set_local(root, m1);
    //  transform_manager.update_all();
    std::cout << "aaaa: " << (clock() - t3) << "\n";

    for (auto c : entities)
    {
        auto m2 = transform_manager.get_world(c);
        mats.push_back(m2);
    }

    auto t4 = clock();

    std::cout << "test_transform4: " << (t4 - t3) << "\n";
    std::cout << "total: " << (t4 - t1) << "\n";
}


void test_transform3()
{
    Entity c1 = entity_manager.create();

    Entity c2 = entity_manager.create();
    Entity c3 = entity_manager.create();
    Entity c4 = entity_manager.create();

    entity_manager.set_parent(c2, c1);
    entity_manager.set_parent(c3, c2);
    entity_manager.set_parent(c4, c1);
}

extern EntityManager entity_manager;
extern TransformManager transform_manager;

osg::Vec3 get_translation(const osg::Matrix& m) { return m.getTrans(); }

void test_transform_basic()
{
    std::cout << "==== test_transform_basic ====" << std::endl;

    entity_manager.init(10);
    transform_manager.set_entity_manager(&entity_manager);
    transform_manager.init(10);

    // 创建3个entity
    Entity root = entity_manager.create();
    Entity child = entity_manager.create();
    Entity grandchild = entity_manager.create();

    // 建立层级关系
    entity_manager.set_parent(child, root);
    entity_manager.set_parent(grandchild, child);

    // 设置 local transform
    transform_manager.set_local(root, osg::Matrix::translate(10, 0, 0));
    transform_manager.set_local(child, osg::Matrix::translate(0, 5, 0));
    transform_manager.set_local(grandchild, osg::Matrix::translate(0, 0, 2));

    // 获取 world
    osg::Vec3 r = get_translation(transform_manager.get_world(root));
    osg::Vec3 c = get_translation(transform_manager.get_world(child));
    osg::Vec3 g = get_translation(transform_manager.get_world(grandchild));

    std::cout << "root: " << r.x() << "," << r.y() << "," << r.z() << std::endl;
    std::cout << "child: " << c.x() << "," << c.y() << "," << c.z() << std::endl;
    std::cout << "grandchild: " << g.x() << "," << g.y() << "," << g.z()
        << std::endl;

    // 断言
    assert(r == osg::Vec3(10, 0, 0));
    assert(c == osg::Vec3(10, 5, 0));
    assert(g == osg::Vec3(10, 5, 2));

    std::cout << "PASS\n";
}

void test_dirty_propagation()
{
    std::cout << "==== test_dirty_propagation ====" << std::endl;

    Entity root = entity_manager.create();
    Entity root2 = entity_manager.create();
    Entity child = entity_manager.create();

    entity_manager.set_parent(child, root);

    transform_manager.set_local(root, osg::Matrix::translate(1, 0, 0));
    transform_manager.set_local(child, osg::Matrix::translate(0, 1, 0));

    // 先更新一次
    transform_manager.update_all();

    osg::Vec3 before = get_translation(transform_manager.get_world(child));
    assert(before == osg::Vec3(1, 1, 0));

    // 修改父节点
    transform_manager.set_local(root, osg::Matrix::translate(5, 0, 0));

    osg::Vec3 after = get_translation(transform_manager.get_world(child));

    std::cout << "before: " << before.x() << "," << before.y() << ","
        << before.z() << std::endl;
    std::cout << "after: " << after.x() << "," << after.y() << "," << after.z()
        << std::endl;

    // 应该自动更新
    assert(after == osg::Vec3(5, 1, 0));

    entity_manager.set_parent(root, root2);
    transform_manager.set_local(root2, osg::Matrix::translate(100, 0, 0));
    osg::Vec3 after2 = get_translation(transform_manager.get_world(child));
    assert(after2 == osg::Vec3(105, 1, 0));

    entity_manager.set_parent(root, {INVALID_ID, 0});
    osg::Vec3 after3 = get_translation(transform_manager.get_world(child));
    assert(after3 == osg::Vec3(5, 1, 0));

    std::cout << "PASS\n";
}

//----------------------------------------------------render info test
void test_visibility_propagation()
{
    std::cout << "==== test_visibility_propagation ====\n";

    Entity root = entity_manager.create();
    Entity child = entity_manager.create();

    entity_manager.set_parent(child, root);

    // root 不可见
    render_info_manager.set_local_visible(root, false);

    auto w = render_info_manager.get_world(child);

    std::cout << "child.visible = " << w.visible << std::endl;

    assert(w.visible == 0);
    render_info_manager.set_local_visible(root, true);

    auto w2 = render_info_manager.get_world(child);

    std::cout << "child.visible = " << w2.visible << std::endl;

    assert(w2.visible == 1);


    render_info_manager.set_local_visible(root, false);
    auto w3_ = render_info_manager.get_world(child);

    std::cout << "child.visible = " << w3_.visible << std::endl;

    assert(w3_.visible == 0);

    entity_manager.set_parent(child, {INVALID_ID, 0});

    auto w3 = render_info_manager.get_world(child);

    std::cout << "child.visible = " << w3.visible << std::endl;

    assert(w3.visible == 1);

    std::cout << "PASS\n";
}

void test_render_info_dirty_propagation()
{
    std::cout << "==== test_dirty_propagation ====\n";

    Entity root = entity_manager.create();
    Entity child = entity_manager.create();

    entity_manager.set_parent(child, root);

    render_info_manager.set_local_layer(root, 1);

    auto before = render_info_manager.get_world(child);
    assert(before.layer == 1);

    // 修改父节点
    render_info_manager.set_local_layer(root, 9);

    auto after = render_info_manager.get_world(child);

    std::cout << "before: " << before.layer << "\n";
    std::cout << "after: " << after.layer << "\n";

    assert(after.layer == 9);

    std::cout << "PASS\n";
}

void test_deep_hierarchy()
{
    std::cout << "==== test_deep_hierarchy ====\n";

    Entity a = entity_manager.create();
    Entity b = entity_manager.create();
    Entity c = entity_manager.create();

    entity_manager.set_parent(b, a);
    entity_manager.set_parent(c, b);

    render_info_manager.set_local_category(a, 2);

    auto w = render_info_manager.get_world(c);

    std::cout << "c.category = " << w.category << std::endl;

    assert(w.category == 2);

    std::cout << "PASS\n";
}

void test_mask_behavior()
{
    std::cout << "==== test_mask_behavior ====\n";

    Entity root = entity_manager.create();
    Entity child = entity_manager.create();

    entity_manager.set_parent(child, root);

    render_info_manager.set_local_lineWidth(root, 2);

    // child 没设置 → 应继承
    auto w1 = render_info_manager.get_world(child);
    assert(w1.lineWidth == 2);

    // child 设置 → 覆盖
    render_info_manager.set_local_lineWidth(child, 5);

    auto w2 = render_info_manager.get_world(child);
    assert(w2.lineWidth == 5);

    std::cout << "PASS\n";
}

void test_ecs2()
{
    test_transform_basic();
    test_dirty_propagation();

    test_visibility_propagation();
    test_render_info_dirty_propagation();
    test_deep_hierarchy();
    test_mask_behavior();

    std::cout << "aaaa ";
}
