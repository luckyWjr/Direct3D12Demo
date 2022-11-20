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
// �������ֵHRESULT < 0���׳��쳣
#define ThrowIfFailed(x)                                                    \
{                                                                           \
    HRESULT hr__ = (x);                                                     \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, __FILEW__, __LINE__); } \
}
#endif

// DrawIndexedInstanced��Ӧ�Ĳ���
struct Submesh
{
	UINT indexCount = 0;			// �����index����
	UINT startIndexLocation = 0;	// ��index buffer�е���ʼλ��
	INT baseVertexLocation = 0;		// ƫ����
};

struct Mesh
{
	std::string name;

	// ������±���Ϣ��CPU�е��ڴ�顣ʹ��ʱ������ʵ�����ǿת��std::vector<Vertex>��std::vector<std::uint16_t>��std::vector<std::uint32_t>��
	ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
	ComPtr<ID3DBlob> indexBufferCPU = nullptr;

	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

	// ͨ��Upload heap���Resource��cpu���ݴ���default heap��
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	UINT vertexByteStride = 0;											// vertex bufferÿ��Ԫ��ռ�õ��ֽڴ�С
	UINT vertexBufferSize = 0;											// vertex buffer�ܹ�ռ�õ��ֽڴ�С
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;						// index buffer��ÿ��Ԫ�ص�format��DXGI_FORMAT_R16_UINT �� DXGI_FORMAT_R32_UINT��
	UINT indexBufferSize = 0;											// index buffer�ܹ�ռ�õ��ֽڴ�С

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

	// ��upload heap���䵽default deap�󣬿��Խ�upload heap�е�Resource�ͷŵ�
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

	// albedo texture���±�
	int albedoTextureIndex = -1;
};

// ����constant buffer
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