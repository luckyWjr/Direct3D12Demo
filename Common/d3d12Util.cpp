#include "d3d12Util.h"
#include <comdef.h>
#include <fstream>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

std::wstring DxException::ToString()const
{
    // ����HRESULTֵ����ȡ�������顣_com_error��Ҫinclude comdef.h
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();
    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

ComPtr<ID3DBlob> d3d12Util::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, &blob));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ComPtr<ID3D12Resource> d3d12Util::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const void* data,
    const int size, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&defaultBuffer)));

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // ����Ҫ�����CPU����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = data;
    subResourceData.RowPitch = size;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // ����ǰҪ����resource��state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    // ���Ƚ�CPU�ڴ����ݿ�����upload heap�У�Ȼ����ͨ��ID3D12CommandList::CopySubresourceRegion��upload heap�п�����default buffer��
    UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    return defaultBuffer;
}