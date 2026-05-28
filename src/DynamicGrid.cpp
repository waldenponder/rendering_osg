#include "DynamicGrid .h"
#include <osg/Geometry>

DynamicGrid::DynamicGrid()
{
	_geometry = new osg::Geometry;

	_vertices = new osg::Vec3Array;
	_colors = new osg::Vec4Array;

	_geometry->setVertexArray(_vertices);

	_geometry->setColorArray(_colors);
	_geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	_geometry->addPrimitiveSet(
		new osg::DrawArrays(GL_LINES, 0, 0));

	addDrawable(_geometry);

	_colors->push_back(osg::Vec4(0.7, 0.7, 0.7, 1));

	getOrCreateStateSet()->setMode(
		GL_LIGHTING,
		osg::StateAttribute::OFF);
}



void DynamicGrid::updateGrid(
	const osg::Vec3d& center,
	double step,
	int halfCount)
{
	_vertices->clear();

	double size = step * halfCount;

	double startX =
		floor(center.x() / step) * step;

	double startY =
		floor(center.y() / step) * step;

	for (int i = -halfCount; i <= halfCount; ++i)
	{
		double x = startX + i * step;

		_vertices->push_back(
			osg::Vec3(x, startY - size, 0));

		_vertices->push_back(
			osg::Vec3(x, startY + size, 0));
	}

	for (int j = -halfCount; j <= halfCount; ++j)
	{
		double y = startY + j * step;

		_vertices->push_back(
			osg::Vec3(startX - size, y, 0));

		_vertices->push_back(
			osg::Vec3(startX + size, y, 0));
	}

	auto da =
		static_cast<osg::DrawArrays*>(
			_geometry->getPrimitiveSet(0));

	da->setCount(_vertices->size());

	_vertices->dirty();
	_geometry->dirtyBound();
}
