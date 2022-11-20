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
        Vertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT2& uv) : position(p), normal(n), uv(uv) {}
        Vertex(float px, float py, float pz,
            float nx, float ny, float nz,
            float u, float v) :
            position(px, py, pz), normal(nx, ny, nz), uv(u, v) {}
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 uv;
    };

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint16> indices;
    };

    static MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);
    static MeshData CreateCube(float length);
    static MeshData CreatePyramid(float width, float height);
    static MeshData CreateSphere(float radius, uint16 stackCount, uint16 sliceCount);
};