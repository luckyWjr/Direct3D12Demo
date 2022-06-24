#pragma once
#include "d3d12Util.h"

template<typename T>
class UploadHeapBuffer
{
public:
    UploadHeapBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        m_isConstantBuffer(isConstantBuffer)
    {
        m_elementByteSize = sizeof(T);

        // 如果是constant buffer，则每个元素（每个constant buffer）的大小与 256byte 对齐
        if (isConstantBuffer)
            m_elementByteSize = d3d12Util::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_uploadHeapBuffer)));

        ThrowIfFailed(m_uploadHeapBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
    }

    UploadHeapBuffer(const UploadHeapBuffer& rhs) = delete;
    UploadHeapBuffer& operator=(const UploadHeapBuffer& rhs) = delete;
    ~UploadHeapBuffer()
    {
        if (m_uploadHeapBuffer != nullptr)
            m_uploadHeapBuffer->Unmap(0, nullptr);

        m_mappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return m_uploadHeapBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
    }

protected:
    ComPtr<ID3D12Resource> m_uploadHeapBuffer;
    BYTE* m_mappedData = nullptr;
    UINT m_elementByteSize = 0;
    bool m_isConstantBuffer = false;
};