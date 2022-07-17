#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../Direct3D12Headers/d3d12.h"
#include "../Direct3D12Headers/d3dx12.h"
#include "MathUtil.h"
#include "GeometryManager.h"
#include <wrl.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

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