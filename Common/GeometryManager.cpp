#include "GeometryManager.h"
#include <algorithm>

using namespace DirectX;

GeometryManager::MeshData GeometryManager::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
    MeshData meshData;

    uint32 vertexCount = m * n;
    uint32 faceCount = (m - 1) * (n - 1) * 2;

    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);

    float du = 1.0f / (n - 1);
    float dv = 1.0f / (m - 1);

    meshData.vertices.resize(vertexCount);
    for (uint32 i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dz;
        for (uint32 j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            meshData.vertices[i * n + j].position = XMFLOAT3(x, 0.0f, z);
            meshData.vertices[i * n + j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            meshData.vertices[i * n + j].uv = XMFLOAT2(j * du, i * dv);
        }
    }

    meshData.indices.resize(faceCount * 3);
    uint32 k = 0;
    for (uint32 i = 0; i < m - 1; ++i)
    {
        for (uint32 j = 0; j < n - 1; ++j)
        {
            meshData.indices[k] = i * n + j;
            meshData.indices[k + 1] = i * n + j + 1;
            meshData.indices[k + 2] = (i + 1) * n + j;

            meshData.indices[k + 3] = (i + 1) * n + j;
            meshData.indices[k + 4] = i * n + j + 1;
            meshData.indices[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }

    return meshData;
}

GeometryManager::MeshData GeometryManager::CreateCube(float length)
{
    MeshData meshData;
    Vertex vertices[24];
    uint16 indices[36];
    float halfLength = 0.5f * length;

    // 正面
    vertices[0] = Vertex(-halfLength, -halfLength, -halfLength, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    vertices[1] = Vertex(-halfLength, +halfLength, -halfLength, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    vertices[2] = Vertex(+halfLength, +halfLength, -halfLength, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    vertices[3] = Vertex(+halfLength, -halfLength, -halfLength, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    // 背面
    vertices[4] = Vertex(-halfLength, -halfLength, +halfLength, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    vertices[5] = Vertex(+halfLength, -halfLength, +halfLength, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    vertices[6] = Vertex(+halfLength, +halfLength, +halfLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    vertices[7] = Vertex(-halfLength, +halfLength, +halfLength, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    indices[6] = 4; indices[7] = 5; indices[8] = 6;
    indices[9] = 4; indices[10] = 6; indices[11] = 7;

    // 上面
    vertices[8] = Vertex(-halfLength, +halfLength, -halfLength, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    vertices[9] = Vertex(-halfLength, +halfLength, +halfLength, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    vertices[10] = Vertex(+halfLength, +halfLength, +halfLength, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    vertices[11] = Vertex(+halfLength, +halfLength, -halfLength, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    indices[12] = 8; indices[13] = 9; indices[14] = 10;
    indices[15] = 8; indices[16] = 10; indices[17] = 11;

    // 下面
    vertices[12] = Vertex(-halfLength, -halfLength, -halfLength, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
    vertices[13] = Vertex(+halfLength, -halfLength, -halfLength, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
    vertices[14] = Vertex(+halfLength, -halfLength, +halfLength, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
    vertices[15] = Vertex(-halfLength, -halfLength, +halfLength, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
    indices[18] = 12; indices[19] = 13; indices[20] = 14;
    indices[21] = 12; indices[22] = 14; indices[23] = 15;

    // 左面
    vertices[16] = Vertex(-halfLength, -halfLength, +halfLength, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    vertices[17] = Vertex(-halfLength, +halfLength, +halfLength, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[18] = Vertex(-halfLength, +halfLength, -halfLength, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[19] = Vertex(-halfLength, -halfLength, -halfLength, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    indices[24] = 16; indices[25] = 17; indices[26] = 18;
    indices[27] = 16; indices[28] = 18; indices[29] = 19;

    // 右面
    vertices[20] = Vertex(+halfLength, -halfLength, -halfLength, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    vertices[21] = Vertex(+halfLength, +halfLength, -halfLength, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[22] = Vertex(+halfLength, +halfLength, +halfLength, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[23] = Vertex(+halfLength, -halfLength, +halfLength, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    indices[30] = 20; indices[31] = 21; indices[32] = 22;
    indices[33] = 20; indices[34] = 21; indices[35] = 23;

    meshData.vertices.assign(&vertices[0], &vertices[24]);
    meshData.indices.assign(&indices[0], &indices[36]);

    return meshData;
}

GeometryManager::MeshData GeometryManager::CreatePyramid(float width, float height)
{
    MeshData meshData;
    Vertex vertices[16];
    uint16 indices[18];
    float halfWidth = 0.5f * width;

    // 五个顶点的坐标
    XMFLOAT3 top(0.0f, height, 0.0f);                   //顶点
    XMFLOAT3 leftFront(-halfWidth, 0.0f, -halfWidth);   //左前
    XMFLOAT3 leftBack(-halfWidth, 0.0f, +halfWidth);    //左后
    XMFLOAT3 rightFront(+halfWidth, 0.0f, -halfWidth);  //右前
    XMFLOAT3 rightBack(+halfWidth, 0.0f, +halfWidth);   //右后

    XMFLOAT2 topUV(0.5f, 0.0f);
    XMFLOAT2 leftUV(0.0f, 1.0f);
    XMFLOAT2 rightUV(1.0f, 1.0f);

    XMFLOAT3 normalFloat3;
    XMVECTOR normalVector;

    // 正面三角形
    normalVector = MathUtil::ComputeNormal(XMLoadFloat3(&top), XMLoadFloat3(&rightFront), XMLoadFloat3(&leftFront));
    XMStoreFloat3(&normalFloat3, normalVector);
    vertices[0] = Vertex(top, normalFloat3, topUV);
    vertices[1] = Vertex(rightFront, normalFloat3, rightUV);
    vertices[2] = Vertex(leftFront, normalFloat3, leftUV);
    indices[0] = 0; indices[1] = 1; indices[2] = 2;

    // 背面三角形
    normalVector = MathUtil::ComputeNormal(XMLoadFloat3(&top), XMLoadFloat3(&leftBack), XMLoadFloat3(&rightBack));
    XMStoreFloat3(&normalFloat3, normalVector);
    vertices[3] = Vertex(top, normalFloat3, topUV);
    vertices[4] = Vertex(leftBack, normalFloat3, rightUV);
    vertices[5] = Vertex(rightBack, normalFloat3, leftUV);
    indices[3] = 3; indices[4] = 4; indices[5] = 5;

    // 左面三角形
    normalVector = MathUtil::ComputeNormal(XMLoadFloat3(&top), XMLoadFloat3(&leftFront), XMLoadFloat3(&leftBack));
    XMStoreFloat3(&normalFloat3, normalVector);
    vertices[6] = Vertex(top, normalFloat3, topUV);
    vertices[7] = Vertex(leftFront, normalFloat3, rightUV);
    vertices[8] = Vertex(leftBack, normalFloat3, leftUV);
    indices[6] = 6; indices[7] = 7; indices[8] = 8;

    // 右面三角形
    normalVector = MathUtil::ComputeNormal(XMLoadFloat3(&top), XMLoadFloat3(&rightBack), XMLoadFloat3(&rightFront));
    XMStoreFloat3(&normalFloat3, normalVector);
    vertices[9] = Vertex(top, normalFloat3, topUV);
    vertices[10] = Vertex(rightBack, normalFloat3, rightUV);
    vertices[11] = Vertex(rightFront, normalFloat3, leftUV);
    indices[9] = 9; indices[10] = 10; indices[11] = 11;

    // 下面两个三角形
    normalFloat3 = XMFLOAT3(0.0f, -1.0f, 0.0f);
    vertices[12] = Vertex(leftFront, normalFloat3, { 1.0f, 1.0f });
    vertices[13] = Vertex(rightFront, normalFloat3, { 0.0f, 1.0f });
    vertices[14] = Vertex(rightBack, normalFloat3, { 0.0f, 0.0f });
    vertices[15] = Vertex(leftFront, normalFloat3, { 1.0f, 0.0f });
    indices[12] = 12; indices[13] = 13; indices[14] = 14;
    indices[15] = 12; indices[16] = 14; indices[17] = 15;

    meshData.vertices.assign(&vertices[0], &vertices[16]);
    meshData.indices.assign(&indices[0], &indices[18]);

    return meshData;
}

GeometryManager::MeshData GeometryManager::CreateSphere(float radius, uint16 stackCount, uint16 sliceCount)
{
    MeshData meshData;

    Vertex topVertex(0.0f, radius, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, 0.0f);       // 北极点
    Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);   // 南极点

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

            XMVECTOR normal = XMLoadFloat3(&v.position);
            XMStoreFloat3(&v.normal, XMVector3Normalize(normal));

            v.uv.x = sliceRadian / XM_2PI;
            v.uv.y = stackRadian / XM_PI;

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
