#include "ViewerWindow.h"

#include <BRepCheck_Analyzer.hxx>
#include <BRepLib_ToolTriangulatedShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/LightModel>
#include <osg/Material>

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <string>

// OCCT owns the CAD geometry. OSG only receives its tessellated triangles.
osg::ref_ptr<osg::Geode> toOsg(const TopoDS_Shape& shape,
                              const osg::Vec4& color, const std::string& name)
{
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid())
        throw std::runtime_error(name + ": invalid OCCT solid");

    BRepMesh_IncrementalMesh mesh(shape, 0.02, false, 0.12, true);
    if (!mesh.IsDone())
        throw std::runtime_error(name + ": tessellation failed");

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    unsigned faceCount = 0;
    for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next())
    {
        const TopoDS_Face face = TopoDS::Face(it.Current());
        TopLoc_Location location;
        const auto triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull() || triangulation->NbTriangles() == 0)
            throw std::runtime_error(name + ": face has no triangles");
        BRepLib_ToolTriangulatedShape::ComputeNormals(face, triangulation);
        const gp_Trsf transform = location.Transformation();
        const bool reversed = face.Orientation() == TopAbs_REVERSED;
        const bool flipWinding = reversed != transform.IsNegative();
        const unsigned base = static_cast<unsigned>(vertices->size());
        for (int i = 1; i <= triangulation->NbNodes(); ++i)
        {
            const gp_Pnt p = triangulation->Node(i).Transformed(transform);
            gp_Dir n = triangulation->Normal(i).Transformed(transform);
            if (reversed) n.Reverse();
            vertices->push_back(osg::Vec3(p.X(), p.Y(), p.Z()));
            normals->push_back(osg::Vec3(n.X(), n.Y(), n.Z()));
        }
        for (int i = 1; i <= triangulation->NbTriangles(); ++i)
        {
            int a, b, c;
            triangulation->Triangle(i).Get(a, b, c);
            if (flipWinding) std::swap(b, c);
            indices->push_back(base + a - 1);
            indices->push_back(base + b - 1);
            indices->push_back(base + c - 1);
        }
        ++faceCount;
    }
    if (indices->empty()) throw std::runtime_error(name + ": empty mesh");

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(indices);
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.25f);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.45f, 0.45f, 0.45f, 1));
    material->setShininess(osg::Material::FRONT_AND_BACK, 48);
    geometry->getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);
    geometry->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->setName(name);
    geode->addDrawable(geometry);
    std::cout << name << ": " << faceCount << " faces, " << vertices->size()
              << " vertices, " << indices->size() / 3 << " triangles\n";
    return geode;
}

int main(int argc, char** argv)
{
    try
    {
        const bool checkOnly = argc == 2 && std::string(argv[1]) == "--check";
        osg::ref_ptr<osg::Group> scene = new osg::Group;
        scene->addChild(toOsg(BRepPrimAPI_MakeBox(gp_Pnt(-5.5, -1, 0), 2, 2, 2).Shape(),
                             osg::Vec4(0.24f, 0.57f, 0.92f, 1), "Box"));
        scene->addChild(toOsg(BRepPrimAPI_MakeCylinder(
                                 gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 1.15, 2.8).Shape(),
                             osg::Vec4(0.22f, 0.76f, 0.58f, 1), "Cylinder"));
        scene->addChild(toOsg(BRepPrimAPI_MakeCone(
                                 gp_Ax2(gp_Pnt(4.5, 0, 0), gp_Dir(0, 0, 1)), 1.35, 0, 3).Shape(),
                             osg::Vec4(0.98f, 0.62f, 0.22f, 1), "Cone"));
        if (checkOnly)
        {
            std::cout << "PASS: three valid OCCT solids converted to OSG meshes.\n";
            return 0;
        }

        osg::ref_ptr<osg::LightModel> light = new osg::LightModel;
        light->setAmbientIntensity(osg::Vec4(0.28f, 0.28f, 0.32f, 1));
        scene->getOrCreateStateSet()->setAttributeAndModes(light, osg::StateAttribute::ON);
        std::cout << "Left drag: rotate | Middle drag: pan | Wheel: zoom | Space: reset | Esc: exit\n";
        return runViewer(argc, argv, *scene);
    }
    catch (const Standard_Failure& error)
    {
        std::cerr << "OCCT error: " << error.GetMessageString() << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}
