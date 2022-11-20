//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

//#include <cstddef>
//#include <cstdint>

// for d3d12
#include "../Direct3D12Headers/d3d12.h"
#include "../Direct3D12Headers/d3dx12.h"
#include <wrl.h>

namespace DirectX
{
#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
    enum DDS_ALPHA_MODE : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
#endif

    // Standard version
    HRESULT __cdecl CreateDDSTextureFromMemory12(_In_ ID3D12Device* device,
        _In_ ID3D12GraphicsCommandList* cmdList,
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _Out_ Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        _Out_ Microsoft::WRL::ComPtr<ID3D12Resource>& textureUploadHeap,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);

    HRESULT __cdecl CreateDDSTextureFromFile12(
        _In_ ID3D12Device* device,
        _In_ ID3D12GraphicsCommandList* cmdList,
        _In_z_ const wchar_t* szFileName,
        _Out_ Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        _Out_ Microsoft::WRL::ComPtr<ID3D12Resource>& textureUploadHeap,
        _In_ size_t maxsize = 0,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr);
}
