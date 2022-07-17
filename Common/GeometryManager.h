#pragma once

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

class GeometryManager
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    struct Vertex
    {
        Vertex() {}
        Vertex(const XMFLOAT3& p) :position(p) {}
        Vertex(float px, float py, float pz) : position(px, py, pz) {}
        XMFLOAT3 position;
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