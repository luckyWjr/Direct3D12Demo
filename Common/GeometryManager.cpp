#include "GeometryManager.h"
#include <algorithm>

using namespace DirectX;

GeometryManager::MeshData GeometryManager::CreateCube(float length)
{
    MeshData meshData;
    Vertex vertices[24];
    uint16 indices[36];
    float halfLength = 0.5f * length;

    // 正面
    vertices[0] = Vertex(-halfLength, -halfLength, -halfLength);
    vertices[1] = Vertex(-halfLength, +halfLength, -halfLength);
    vertices[2] = Vertex(+halfLength, +halfLength, -halfLength);
    vertices[3] = Vertex(+halfLength, -halfLength, -halfLength);
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    // 背面
    vertices[4] = Vertex(-halfLength, -halfLength, +halfLength);
    vertices[5] = Vertex(+halfLength, -halfLength, +halfLength);
    vertices[6] = Vertex(+halfLength, +halfLength, +halfLength);
    vertices[7] = Vertex(-halfLength, +halfLength, +halfLength);
    indices[6] = 4; indices[7] = 5; indices[8] = 6;
    indices[9] = 4; indices[10] = 6; indices[11] = 7;

    // 上面
    vertices[8] = Vertex(-halfLength, +halfLength, -halfLength);
    vertices[9] = Vertex(-halfLength, +halfLength, +halfLength);
    vertices[10] = Vertex(+halfLength, +halfLength, +halfLength);
    vertices[11] = Vertex(+halfLength, +halfLength, -halfLength);
    indices[12] = 8; indices[13] = 9; indices[14] = 10;
    indices[15] = 8; indices[16] = 10; indices[17] = 11;

    // 下面
    vertices[12] = Vertex(-halfLength, -halfLength, -halfLength);
    vertices[13] = Vertex(+halfLength, -halfLength, -halfLength);
    vertices[14] = Vertex(+halfLength, -halfLength, +halfLength);
    vertices[15] = Vertex(-halfLength, -halfLength, +halfLength);
    indices[18] = 12; indices[19] = 13; indices[20] = 14;
    indices[21] = 12; indices[22] = 14; indices[23] = 15;

    // 左面
    vertices[16] = Vertex(-halfLength, -halfLength, +halfLength);
    vertices[17] = Vertex(-halfLength, +halfLength, +halfLength);
    vertices[18] = Vertex(-halfLength, +halfLength, -halfLength);
    vertices[19] = Vertex(-halfLength, -halfLength, -halfLength);
    indices[24] = 16; indices[25] = 17; indices[26] = 18;
    indices[27] = 16; indices[28] = 18; indices[29] = 19;

    // 右面
    vertices[20] = Vertex(+halfLength, -halfLength, -halfLength);
    vertices[21] = Vertex(+halfLength, +halfLength, -halfLength);
    vertices[22] = Vertex(+halfLength, +halfLength, +halfLength);
    vertices[23] = Vertex(+halfLength, -halfLength, +halfLength);
    indices[30] = 20; indices[31] = 21; indices[32] = 22;
    indices[33] = 20; indices[34] = 21; indices[35] = 23;

    meshData.vertices.assign(&vertices[0], &vertices[24]);
    meshData.indices.assign(&indices[0], &indices[36]);

    return meshData;
}

GeometryManager::MeshData GeometryManager::CreatePyramid(float width, float height)
{
    MeshData meshData;
    Vertex vertices[5];
    uint16 indices[18];
    float halfWidth = 0.5f * width;

    vertices[0] = Vertex(0, height, 0);                 //顶点
    vertices[1] = Vertex(-halfWidth, 0, -halfWidth);    //左前
    vertices[2] = Vertex(-halfWidth, 0, +halfWidth);    //左后
    vertices[3] = Vertex(+halfWidth, 0, -halfWidth);    //右前
    vertices[4] = Vertex(+halfWidth, 0, +halfWidth);    //右后

    indices[0] = 0; indices[1] = 3; indices[2] = 1;     // 正面
    indices[3] = 0; indices[4] = 2; indices[5] = 4;     // 背面
    indices[6] = 0; indices[7] = 1; indices[8] = 2;     // 左面
    indices[9] = 0; indices[10] = 4; indices[11] = 3;   // 右面
    indices[12] = 1; indices[13] = 4; indices[14] = 2;  // 下面
    indices[15] = 1; indices[16] = 3; indices[17] = 4;  // 下面

    meshData.vertices.assign(&vertices[0], &vertices[5]);
    meshData.indices.assign(&indices[0], &indices[18]);

    return meshData;
}

GeometryManager::MeshData GeometryManager::CreateSphere(float radius, uint16 stackCount, uint16 sliceCount)
{
    MeshData meshData;

    Vertex topVertex(0.0f, radius, 0.0f);       // 北极点
    Vertex bottomVertex(0.0f, -radius, 0.0f);   // 南极点

    meshData.vertices.push_back(topVertex);

    float radianOfPerStack = XM_PI / stackCount;
    float radianOfPerSlice = XM_2PI / sliceCount;

    // 一圈圈自上向下，创建顶点
    for (uint16 i = 1; i <= stackCount - 1; ++i)
    {
        float stackRadian = radianOfPerStack * i;
        for (uint16 j = 0; j <= sliceCount; ++j)
        {
            float sliceRadian = radianOfPerSlice * j;

            Vertex v;
            v.position.x = radius * sinf(stackRadian) * cosf(sliceRadian);
            v.position.y = radius * cosf(stackRadian);
            v.position.z = radius * sinf(stackRadian) * sinf(sliceRadian);

            meshData.vertices.push_back(v);
        }
    }

    meshData.vertices.push_back(bottomVertex);

    // 最上面一层stack，每个三角形共用topVertex
    for (uint16 i = 1; i <= sliceCount; ++i)
    {
        meshData.indices.push_back(0);
        meshData.indices.push_back(i + 1);
        meshData.indices.push_back(i);
    }

    uint16 baseIndex = 1;
    uint16 vertexCountPerRing = sliceCount + 1;   // 每根纬线上的顶点数量
    for (uint16 i = 0; i < stackCount - 2; ++i)
    {
        for (uint16 j = 0; j < sliceCount; ++j)
        {
            meshData.indices.push_back(baseIndex + i * vertexCountPerRing + j);
            meshData.indices.push_back(baseIndex + i * vertexCountPerRing + j + 1);
            meshData.indices.push_back(baseIndex + (i + 1) * vertexCountPerRing + j);

            meshData.indices.push_back(baseIndex + (i + 1) * vertexCountPerRing + j);
            meshData.indices.push_back(baseIndex + i * vertexCountPerRing + j + 1);
            meshData.indices.push_back(baseIndex + (i + 1) * vertexCountPerRing + j + 1);
        }
    }

    // 最下面一层stack，每个三角形共用bottomVertex
    uint16 bottomVertexIndex = (uint16)meshData.vertices.size() - 1;
    baseIndex = bottomVertexIndex - vertexCountPerRing;
    for (uint16 i = 0; i < sliceCount; ++i)
    {
        meshData.indices.push_back(bottomVertexIndex);
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(baseIndex + i + 1);
    }

    return meshData;
}