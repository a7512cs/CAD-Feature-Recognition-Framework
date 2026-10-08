#pragma once

#include <vector>

// Framework-owned geometry, produced once by an importer (ADR-0001).
// Schematic for the POC; real fields land with a real importer.
// Facts only: derived conclusions (e.g. edge convexity) belong to AAG.

enum class SurfaceType
{
    Plane,
    Cylinder,
    Cone,
    Sphere,
    Torus,
    Nurbs
};

struct Face
{
    int id = 0;
    SurfaceType surface = SurfaceType::Plane;
    double area = 0.0;
};

struct Edge
{
    int id = 0;
};

struct Trim
{
    int id = 0;
};

struct Vertex
{
    int id = 0;
};

struct BRep
{
    std::vector<Face> faces;
    std::vector<Edge> edges;
    std::vector<Trim> trims;
    std::vector<Vertex> vertices;
};
