
#include <vector>
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <iostream>

class CollectDrawableVisitor : public osg::NodeVisitor {
public:
  // 构造函数：设置遍历模式（TRAVERSE_ALL_CHILDREN 遍历所有子节点）
  CollectDrawableVisitor()
      : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

  // 覆写apply(Geode&)：Geode是唯一挂载Drawable的节点类型
  void apply(osg::Geode &geode) override {
    // 遍历Geode中所有的Drawable
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
      osg::ref_ptr<osg::Geometry> drawable =
          dynamic_cast<osg::Geometry *>(geode.getDrawable(i));
      if (drawable) // 判空，避免无效指针
      {
        _drawables.push_back(drawable);
        // 可选：打印Drawable信息（调试用）
        std::cout << "drawable " << drawable->className() << std::endl;
      }
    }

    // 继续遍历Geode的子节点（虽然Geode通常无直接子节点，但保留遍历逻辑）
    traverse(geode);
  }

  // 覆写apply(Node&)：处理普通Group/Node节点，继续遍历其子节点
  void apply(osg::Node &node) override {
    traverse(node); // 必须调用traverse，否则子节点不会被遍历
  }

  // 获取收集到的所有Drawable
  std::vector<osg::ref_ptr<osg::Geometry>> &getDrawables() {
    return _drawables;
  }

  // 清空收集结果（可重复使用Visitor）
  void clear() { _drawables.clear(); }


  // 存储收集到的Drawable（使用osg::ref_ptr避免内存泄漏）
  std::vector<osg::ref_ptr<osg::Geometry>> _drawables;
};
