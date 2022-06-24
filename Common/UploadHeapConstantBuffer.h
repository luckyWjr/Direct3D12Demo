#pragma once
#include "UploadHeapBuffer.h"

template<typename T>
class UploadHeapConstantBuffer : public UploadHeapBuffer<T>
{
public:
    UploadHeapConstantBuffer(ID3D12Device* device, UINT elementCount) :
        UploadHeapBuffer<T>(device, elementCount, true)
    {

    }

    // ´´½¨CBV
    void CreateConstantBufferView(ID3D12Device* device, const CD3DX12_CPU_DESCRIPTOR_HANDLE& cbvHeapHandle)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = m_uploadHeapBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = m_elementByteSize;
        device->CreateConstantBufferView(&cbvDesc, cbvHeapHandle);
    }
};