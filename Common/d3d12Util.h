#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../Direct3D12Headers/d3d12.h"
#include "../Direct3D12Headers/d3dx12.h"
#include "../DirectXTK//DDSTextureLoader.h"
#include <wrl.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <unordered_map>
#include <array>

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

// DrawIndexedInstanced对应的参数
struct Submesh
{
	UINT indexCount = 0;			// 物体的index数量
	UINT startIndexLocation = 0;	// 在index buffer中的起始位置
	INT baseVertexLocation = 0;		// 偏移量
};

struct Mesh
{
	std::string name;

	// 顶点和下标信息在CPU中的内存块。使用时，根据实际情况强转成std::vector<Vertex>、std::vector<std::uint16_t>、std::vector<std::uint32_t>等
	ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
	ComPtr<ID3DBlob> indexBufferCPU = nullptr;

	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

	// 通过Upload heap里的Resource将cpu数据传到default heap中
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	UINT vertexByteStride = 0;											// vertex buffer每个元素占用的字节大小
	UINT vertexBufferSize = 0;											// vertex buffer总共占用的字节大小
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;						// index buffer中每个元素的format（DXGI_FORMAT_R16_UINT 或 DXGI_FORMAT_R32_UINT）
	UINT indexBufferSize = 0;											// index buffer总共占用的字节大小

	std::unordered_map<std::string, Submesh> drawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = vertexByteStride;
		vbv.SizeInBytes = vertexBufferSize;
		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = indexFormat;
		ibv.SizeInBytes = indexBufferSize;
		return ibv;
	}

	// 从upload heap传输到default deap后，可以将upload heap中的Resource释放掉
	void DisposeUploaders()
	{
		vertexBufferUploader = nullptr;
		indexBufferUploader = nullptr;
	}
};

struct Material
{
    std::string name;
    int cbIndex = -1;
    DirectX::XMFLOAT4 albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
    float roughness = 0.25f;

	// albedo texture的下标
	int albedoTextureIndex = -1;
};

// 用作constant buffer
struct MaterialConstant
{
    DirectX::XMFLOAT4 albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
    float roughness = 0.25f;
};

struct Texture
{
	std::string name;
	std::wstring fileName;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploader = nullptr;
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