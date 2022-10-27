#pragma once

#include <DirectXMath.h>
#include <vector>
#include "MathUtil.h"

using namespace DirectX;

class GeometryManager
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    struct Vertex
    {
        Vertex() {}
        Vertex(const XMFLOAT3& p, const XMFLOAT3& n) : position(p), normal(n) {}
        Vertex(float px, float py, float pz,
            float nx, float ny, float nz) :
            position(px, py, pz), normal(nx, ny, nz) {}
        Vertex(float px, float py, float pz) :
            position(px, py, pz), normal(0, 1, 0) {}
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint16> indices;
    };

    static MeshData CreateCube(float length);
    static MeshData CreatePyramid(float width, float height);
    static MeshData CreateSphere(float radius, uint16 stackCount, uint16 sliceCount);
};