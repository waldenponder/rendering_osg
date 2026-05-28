#pragma once
#include "osg/Geode"

class DynamicGrid : public osg::Geode
{
public:
	DynamicGrid();

	void updateGrid(
		const osg::Vec3d& center,
		double step,
		int halfCount);

private:
	osg::ref_ptr<osg::Geometry> _geometry;
	osg::ref_ptr<osg::Vec3Array> _vertices;
	osg::ref_ptr<osg::Vec4Array> _colors;
};
