#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../Direct3D12Headers/d3d12.h"
#include "../Direct3D12Headers/d3dx12.h"
#include <wrl.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include "MathUtil.h"
#include "GeometryManager.h"

using Microsoft::WRL::ComPtr;

class d3d12Util
{
public:

    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }

    static ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);
    static ComPtr<ID3D12Resource> CreateDefaultHeapBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const void* data,
        const int size, ComPtr<ID3D12Resource>& uploadBuffer);
    static std::wstring GetLatestWinPixGpuCapturerPath();
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

#ifndef ThrowIfFailed
// 如果返回值HRESULT < 0，抛出异常
#define ThrowIfFailed(x)                                                    \
{                                                                           \
    HRESULT hr__ = (x);                                                     \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, __FILEW__, __LINE__); } \
}
#endif

struct Material
{
    std::string name;
    int cbIndex = -1;
    DirectX::XMFLOAT4 albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
    float roughness = 0.25f;
};

// 用作constant buffer
struct MaterialConstant
{
    DirectX::XMFLOAT4 albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
    float roughness = 0.25f;
};

#define MAX_LIGHT_COUNT 16

struct Light
{
    DirectX::XMFLOAT3 strength;     // Light color
    float falloffStart;             // point/spot light only
    DirectX::XMFLOAT3 direction;    // directional/spot light only
    float falloffEnd;               // point/spot light only
    DirectX::XMFLOAT3 Position;     // point/spot light only
    float spotPower;                // spot light only
};