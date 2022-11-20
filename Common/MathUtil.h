#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class MathUtil
{
public:
    // 单位矩阵
    static XMFLOAT4X4 Identity4x4()
    {
        static XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    // 计算单个三角形的法线
    static XMVECTOR ComputeNormal(FXMVECTOR p0, FXMVECTOR p1, FXMVECTOR p2)
    {
        XMVECTOR u = p1 - p0;
        XMVECTOR v = p2 - p0;
        return XMVector3Normalize(XMVector3Cross(u, v));
    }

    // 计算M矩阵的逆矩阵的转置矩阵
    static XMMATRIX InverseTranspose(CXMMATRIX M)
    {
        XMMATRIX A = M;
        // 忽略平移操作
        A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        XMVECTOR det = XMMatrixDeterminant(A);
        return XMMatrixTranspose(XMMatrixInverse(&det, A));
    }

    // 根据旋转矩阵R和缩放矩阵S计算法线变换矩阵
    static XMMATRIX ComputeNormalMatrix(CXMMATRIX R, CXMMATRIX S)
    {
        // 计算缩放矩阵的逆矩阵
        XMMATRIX InverseS = S;
        InverseS.r[0] = XMVectorSet(1.0f / XMVectorGetX(InverseS.r[0]), 0.0f, 0.0f, 0.0f);
        InverseS.r[1] = XMVectorSet(0.0f, 1.0f / XMVectorGetY(InverseS.r[1]), 0.0f, 0.0f);
        InverseS.r[2] = XMVectorSet(0.0f, 0.0f, 1.0f / XMVectorGetZ(InverseS.r[2]), 0.0f);

        return XMMatrixMultiply(R, InverseS);
    }
};