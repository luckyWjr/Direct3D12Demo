#include <windows.h>
#include "../Common/d3d12Util.h"
#include "../Common/UploadHeapConstantBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// ��������
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

struct ObjectConstant
{
    XMFLOAT4X4 modelMatrix = MathUtil::Identity4x4();
    XMFLOAT4X4 normalMatrix = MathUtil::Identity4x4();
};

struct PassConstant
{
    XMFLOAT4X4 viewMatrix = MathUtil::Identity4x4();
    XMFLOAT4X4 projectMatrix = MathUtil::Identity4x4();
    XMFLOAT3 cameraPositionInWorld = { 0.0f, 0.0f, 0.0f };
    float padding0 = 0.0f;
    XMFLOAT4 ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
    Light lights[MAX_LIGHT_COUNT];
};

struct RenderItem
{
    RenderItem() = default;

    // ģ�ͱ任���󣬼����������������µ�λ�ã������Լ���תֵ
    XMFLOAT4X4 modelMatrix = MathUtil::Identity4x4();
    // ��ӦObjectConstant���±�
    UINT objectCBIndex = -1;

    // ��Ӧ��vertex��index buffer��Ϣ
    Mesh* mesh = nullptr;

    // ��Ӧ�Ĳ���
    Material* material = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // ��Ӧmesh�е�Submeshֵ
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    int baseVertexLocation = 0;
};


UINT m_clientWidth = 800, m_clientHeight = 600;     // ���ڳߴ�
HWND m_hwnd;
UINT64 m_fenceValue;
static const UINT m_swapChainBufferCount = 2;
HANDLE m_fenceEvent;
UINT m_rtvDescriptorSize, m_cbvSrvUavDescriptorSize;
UINT m_currendBackBufferIndex = 0;

DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;  // ÿ��Ԫ��ռ��8-bit��ֵ�ķ�Χ[0,1]

ComPtr<ID3D12Device> m_device;
ComPtr<ID3D12CommandQueue> m_commandQueue;
ComPtr<ID3D12CommandAllocator> m_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> m_commandList;
ComPtr<ID3D12Fence> m_fence;
ComPtr<IDXGISwapChain> m_swapChain;
ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
ComPtr<ID3D12Resource> m_swapChainBuffer[m_swapChainBufferCount];
ComPtr<ID3D12RootSignature> m_rootSignature;
std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
std::vector<std::unique_ptr<RenderItem>> m_renderItems;
std::unique_ptr<UploadHeapConstantBuffer<ObjectConstant>> m_objectConstantBuffer;
std::unique_ptr<UploadHeapConstantBuffer<PassConstant>> m_passConstantBuffer;
std::unique_ptr<UploadHeapConstantBuffer<MaterialConstant>> m_materialConstantBuffer;
ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
ComPtr<ID3D12PipelineState> m_pipelineState;

D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissorRect;

std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitDirect3D();
void InitAsset();
void InitMeshes();
void InitRenderItems();
void InitTextures();
void InitMaterials();
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> InitStaticSamplers();
void InitConstantBuffer();
void InitCbvSrvDescriptor();
void FlushCommandQueue();
void CreateRootSignature();
void PopulateCommandList();
void OnUpdate();
void OnRender();
void OnDestroy();


// Ӧ�ó�����ں���
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"TextureMapping",              // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // ���ڴ�С
        m_clientWidth, m_clientHeight,  // ����λ��
        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (m_hwnd == NULL) return 0;

    ShowWindow(m_hwnd, nCmdShow);         // ��ʾ����

    try
    {
        InitDirect3D();
        InitAsset();

        // ��ȡ�¼�����������������̰��������ڱ仯�ȵȡ�
        MSG msg = { };
        while (msg.message != WM_QUIT)      // message loop
        {
            // PeekMessage ��� message queue���Ƿ���message������еĻ����䵽msg�У����ط�0��ֵ
            // PM_REMOVE ����WM_PAINT���͵�message�⣬������message����ִ����PeekMessage���Ƴ� message queue
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);      // �ᴥ�� WindowProc ����
            }
            else
            {
                OnUpdate();
                OnRender();
            }
        }
        OnDestroy();
    }
    catch (DxException& e)
    {
        OnDestroy();
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

    return 0;
}

// �¼�����
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);     // ��ֹMessage loop
        return 0;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
    // ���� D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
        if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
            LoadLibrary(d3d12Util::GetLatestWinPixGpuCapturerPath().c_str());
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    // ��ֹʹ��alt+enter�л�ȫ��ģʽ
    ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_PRINT_SCREEN));

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
    {
        // ���device����ʧ�ܣ�Fallback �� WARP device.
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Command Queue��Command Allocator��Command List��typeҪһ��
    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = commandListType;                   // Command List������
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;    // Ĭ�ϵ�Command Queue
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    ThrowIfFailed(m_device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&m_commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, commandListType, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 0;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = m_clientWidth;
    sd.BufferDesc.Height = m_clientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = m_backBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;                                // ��������multisample
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = m_swapChainBufferCount;
    sd.OutputWindow = m_hwnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    ThrowIfFailed(factory->CreateSwapChain(m_commandQueue.Get(), &sd, &m_swapChain));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = m_swapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < m_swapChainBufferCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));  // �ᵼ��Swap Chain���buffer�������ü���+1������ʹ����ComPtr�����Զ�����release
        m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);       // ƫ��һ��rtv��С
    }
}

void InitAsset()
{
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC inputLayout = { inputElementDescs, _countof(inputElementDescs) };

    m_commandList->Reset(m_commandAllocator.Get(), nullptr); // ֮ǰclose��command list������Ҫʹ�õ�����Ҫreset�����ſɼ�¼command��

    InitMeshes();
    InitTextures();
    InitMaterials();
    InitRenderItems();
    InitConstantBuffer();
    InitCbvSrvDescriptor();
    CreateRootSignature();

    // shader compiler
    ComPtr<ID3DBlob> vsByteCode = d3d12Util::LoadBinary(L"shaders_vs.cso");
    ComPtr<ID3DBlob> psByteCode = d3d12Util::LoadBinary(L"shaders_ps.cso");

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayout;
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsByteCode.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psByteCode.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_backBufferFormat;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    // ���ڴ�С
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = static_cast<float>(m_clientWidth);
    m_viewport.Height = static_cast<float>(m_clientHeight);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    // �ü�����
    m_scissorRect = { 0, 0, (long)m_clientWidth, (long)m_clientHeight };

    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    m_meshes["baseGeometryMesh"]->DisposeUploaders();
}

void FlushCommandQueue()
{
    if (m_commandQueue == nullptr) return;
    m_fenceValue++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));             // ����һ��ָ�GPU����������fenceֵ

    if (m_fence->GetCompletedValue() < m_fenceValue)                                // ��ȡ��ǰ��fenceֵ
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));   // ��fenceֵ����m_fenceValueʱ���ᴥ��m_fenceEvent
        WaitForSingleObject(m_fenceEvent, INFINITE);                                // �ȴ�m_fenceEvent��ִ��
    }
}

void CreateRootSignature()
{
    // һ��root signature����һ��root parameter����
    // һ��root parameter������root constant��root descriptor��descriptor table
    CD3DX12_ROOT_PARAMETER rootParameters[4];
    CD3DX12_DESCRIPTOR_RANGE passCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);       // register(b0)��������Pass Constant Buffer
    CD3DX12_DESCRIPTOR_RANGE objectCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);     // register(b1)��������Object Constant Buffer
    CD3DX12_DESCRIPTOR_RANGE materialCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);   // register(b2)��������Material Constant Buffer
    CD3DX12_DESCRIPTOR_RANGE textureSrvTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[0].InitAsDescriptorTable(1, &passCbvTable);
    rootParameters[1].InitAsDescriptorTable(1, &objectCbvTable);
    rootParameters[2].InitAsDescriptorTable(1, &materialCbvTable);
    rootParameters[3].InitAsDescriptorTable(1, &textureSrvTable, D3D12_SHADER_VISIBILITY_PIXEL);

    auto staticSamplers = InitStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(4, rootParameters, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (error != nullptr)
        ::OutputDebugStringA((char*)error->GetBufferPointer());
    ThrowIfFailed(hr);
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void InitMeshes()
{
    // ���ּ�����Ķ�����±���Ϣ
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;

    GeometryManager::MeshData cubeMeshData = GeometryManager::CreateCube(1);
    GeometryManager::MeshData pyramidMeshData = GeometryManager::CreatePyramid(1, 2);
    GeometryManager::MeshData sphereMeshData = GeometryManager::CreateSphere(1, 15, 30);

    for (int i = 0; i < cubeMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ cubeMeshData.vertices[i].position, cubeMeshData.vertices[i].normal, cubeMeshData.vertices[i].uv }));
    for (int i = 0; i < pyramidMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ pyramidMeshData.vertices[i].position, pyramidMeshData.vertices[i].normal, pyramidMeshData.vertices[i].uv }));
    for (int i = 0; i < sphereMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ sphereMeshData.vertices[i].position, sphereMeshData.vertices[i].normal, sphereMeshData.vertices[i].uv }));

    indices.insert(indices.end(), std::begin(cubeMeshData.indices), std::end(cubeMeshData.indices));
    indices.insert(indices.end(), std::begin(pyramidMeshData.indices), std::end(pyramidMeshData.indices));
    indices.insert(indices.end(), std::begin(sphereMeshData.indices), std::end(sphereMeshData.indices));

    const UINT verticesSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indicesSize = (UINT)indices.size() * sizeof(std::uint16_t);

    // ������������buffer�е�λ��
    Submesh cubeSubmesh;
    cubeSubmesh.indexCount = (UINT)cubeMeshData.indices.size();
    cubeSubmesh.startIndexLocation = 0;
    cubeSubmesh.baseVertexLocation = 0;

    Submesh pyramidSubmesh;
    pyramidSubmesh.indexCount = (UINT)pyramidMeshData.indices.size();
    pyramidSubmesh.startIndexLocation = (UINT)cubeMeshData.indices.size();
    pyramidSubmesh.baseVertexLocation = (UINT)cubeMeshData.vertices.size();

    Submesh sphereSubmesh;
    sphereSubmesh.indexCount = (UINT)sphereMeshData.indices.size();
    sphereSubmesh.startIndexLocation = pyramidSubmesh.startIndexLocation + (UINT)pyramidMeshData.indices.size();
    sphereSubmesh.baseVertexLocation = pyramidSubmesh.baseVertexLocation + (UINT)pyramidMeshData.vertices.size();

    // ����mesh����
    auto mesh = std::make_unique<Mesh>();
    mesh->name = "baseGeometryMesh";

    // ��vertices��indices���ݿ�����mesh��
    ThrowIfFailed(D3DCreateBlob(verticesSize, &mesh->vertexBufferCPU));
    CopyMemory(mesh->vertexBufferCPU->GetBufferPointer(), vertices.data(), verticesSize);
    ThrowIfFailed(D3DCreateBlob(indicesSize, &mesh->indexBufferCPU));
    CopyMemory(mesh->indexBufferCPU->GetBufferPointer(), indices.data(), indicesSize);

    mesh->vertexBufferGPU = d3d12Util::CreateDefaultHeapBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), verticesSize, mesh->vertexBufferUploader);
    mesh->indexBufferGPU = d3d12Util::CreateDefaultHeapBuffer(m_device.Get(), m_commandList.Get(), indices.data(), indicesSize, mesh->indexBufferUploader);

    mesh->vertexByteStride = sizeof(Vertex);
    mesh->vertexBufferSize = verticesSize;
    mesh->indexFormat = DXGI_FORMAT_R16_UINT;
    mesh->indexBufferSize = indicesSize;

    mesh->drawArgs["cube"] = cubeSubmesh;
    mesh->drawArgs["pyramid"] = pyramidSubmesh;
    mesh->drawArgs["sphere"] = sphereSubmesh;

    m_meshes[mesh->name] = std::move(mesh);
}

void InitTextures()
{
    std::wstring textureFolder = L"../Textures/";
    auto waterTexture = std::make_unique<Texture>();
    waterTexture->name = "water";
    waterTexture->fileName = textureFolder + L"water.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_device.Get(), m_commandList.Get(),
        waterTexture->fileName.c_str(), waterTexture->resource, waterTexture->uploader));

    auto stoneTexture = std::make_unique<Texture>();
    stoneTexture->name = "stone";
    stoneTexture->fileName = textureFolder + L"stone.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_device.Get(), m_commandList.Get(),
        stoneTexture->fileName.c_str(), stoneTexture->resource, stoneTexture->uploader));

    auto grassTexture = std::make_unique<Texture>();
    grassTexture->name = "grass";
    grassTexture->fileName = textureFolder + L"grass.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_device.Get(), m_commandList.Get(),
        grassTexture->fileName.c_str(), grassTexture->resource, grassTexture->uploader));

    m_textures[waterTexture->name] = std::move(waterTexture);
    m_textures[stoneTexture->name] = std::move(stoneTexture);
    m_textures[grassTexture->name] = std::move(grassTexture);
}

void InitMaterials()
{
    auto water = std::make_unique<Material>();
    water->name = "water";
    water->cbIndex = 0;
    water->albedoTextureIndex = 0;
    water->albedo = XMFLOAT4(Colors::White);
    water->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    water->roughness = 0.1f;

    auto stone = std::make_unique<Material>();
    stone->name = "stone";
    stone->cbIndex = 1;
    stone->albedoTextureIndex = 1;
    stone->albedo = XMFLOAT4(Colors::White);
    stone->fresnelR0 = XMFLOAT3(0.8f, 0.8f, 0.8f);
    stone->roughness = 1.0f;

    auto grass = std::make_unique<Material>();
    grass->name = "grass";
    grass->cbIndex = 2;
    grass->albedoTextureIndex = 2;
    grass->albedo = XMFLOAT4(Colors::White);
    grass->fresnelR0 = XMFLOAT3(0.8f, 0.8f, 0.8f);
    grass->roughness = 1.0f;

    m_materials["water"] = std::move(water);
    m_materials["stone"] = std::move(stone);
    m_materials["grass"] = std::move(grass);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> InitStaticSamplers()
{
    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0, D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.0f, 8);

    return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}

void InitRenderItems()
{
    auto cubeItem = std::make_unique<RenderItem>();
    XMMATRIX modelMatrix = XMMATRIX(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0.5f, 0, -0.5f, 1);
    XMStoreFloat4x4(&cubeItem->modelMatrix, modelMatrix);
    cubeItem->objectCBIndex = 0;
    cubeItem->mesh = m_meshes["baseGeometryMesh"].get();
    cubeItem->material = m_materials["water"].get();
    cubeItem->indexCount = cubeItem->mesh->drawArgs["cube"].indexCount;
    cubeItem->startIndexLocation = cubeItem->mesh->drawArgs["cube"].startIndexLocation;
    cubeItem->baseVertexLocation = cubeItem->mesh->drawArgs["cube"].baseVertexLocation;
    m_renderItems.push_back(std::move(cubeItem));

    auto pyramidItem = std::make_unique<RenderItem>();
    modelMatrix = XMMATRIX(
        2, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1.5f, 0,
        2.5f, -1.2f, -0.5f, 1);
    XMStoreFloat4x4(&pyramidItem->modelMatrix, modelMatrix);
    pyramidItem->objectCBIndex = 1;
    pyramidItem->mesh = m_meshes["baseGeometryMesh"].get();
    pyramidItem->material = m_materials["stone"].get();
    pyramidItem->indexCount = pyramidItem->mesh->drawArgs["pyramid"].indexCount;
    pyramidItem->startIndexLocation = pyramidItem->mesh->drawArgs["pyramid"].startIndexLocation;
    pyramidItem->baseVertexLocation = pyramidItem->mesh->drawArgs["pyramid"].baseVertexLocation;
    m_renderItems.push_back(std::move(pyramidItem));

    auto sphereItem = std::make_unique<RenderItem>();
    modelMatrix = XMMATRIX(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 2, 1);
    XMStoreFloat4x4(&sphereItem->modelMatrix, modelMatrix);
    sphereItem->objectCBIndex = 2;
    sphereItem->mesh = m_meshes["baseGeometryMesh"].get();
    sphereItem->material = m_materials["grass"].get();
    sphereItem->indexCount = sphereItem->mesh->drawArgs["sphere"].indexCount;
    sphereItem->startIndexLocation = sphereItem->mesh->drawArgs["sphere"].startIndexLocation;
    sphereItem->baseVertexLocation = sphereItem->mesh->drawArgs["sphere"].baseVertexLocation;
    m_renderItems.push_back(std::move(sphereItem));
}

void InitConstantBuffer()
{
    // ����Pass Constant Buffer����update������ֵ
    m_passConstantBuffer = std::make_unique<UploadHeapConstantBuffer<PassConstant>>(m_device.Get(), 1);

    // ����������Object Constant Buffer
    m_objectConstantBuffer = std::make_unique<UploadHeapConstantBuffer<ObjectConstant>>(m_device.Get(), (UINT)m_renderItems.size());
    for (auto& item : m_renderItems)
    {
        XMMATRIX world = XMLoadFloat4x4(&item->modelMatrix);
        ObjectConstant objectConstant;
        XMStoreFloat4x4(&objectConstant.modelMatrix, XMMatrixTranspose(world));
        m_objectConstantBuffer->CopyData(item->objectCBIndex, objectConstant);
    }

    // ����������Material Constant Buffer
    m_materialConstantBuffer = std::make_unique<UploadHeapConstantBuffer<MaterialConstant>>(m_device.Get(), (UINT)m_materials.size());
    for (auto& e : m_materials)
    {
        Material* mat = e.second.get();
        MaterialConstant materialConstant;
        materialConstant.albedo = mat->albedo;
        materialConstant.fresnelR0 = mat->fresnelR0;
        materialConstant.roughness = mat->roughness;
        m_materialConstantBuffer->CopyData(mat->cbIndex, materialConstant);
    }
}

void InitCbvSrvDescriptor()
{
    UINT passCount = 1;
    UINT itemCount = (UINT)m_renderItems.size();
    UINT materialCount = (UINT)m_materials.size();
    UINT textureCount = (UINT)m_textures.size();

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = passCount + itemCount + materialCount + textureCount;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHeapHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    m_passConstantBuffer->CreateConstantBufferView(m_device.Get(), cbvHeapHandle, 0);

    // Object Constant Buffer��Ӧ��CBV
    for (UINT i = 0; i < itemCount; ++i)
    {
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(i + passCount, m_cbvSrvUavDescriptorSize);
        m_objectConstantBuffer->CreateConstantBufferView(m_device.Get(), handle, i);
    }

    // Material Constant Buffer��Ӧ��CBV
    for (UINT i = 0; i < materialCount; ++i)
    {
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(i + passCount + itemCount, m_cbvSrvUavDescriptorSize);
        m_materialConstantBuffer->CreateConstantBufferView(m_device.Get(), handle, i);
    }

    // Texture��Ӧ��SRV
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    srvHeapHandle.Offset(passCount + itemCount + materialCount, m_cbvSrvUavDescriptorSize);
    for (auto& e : m_textures)
    {
        auto texture = e.second.get()->resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        m_device->CreateShaderResourceView(texture.Get(), &srvDesc, srvHeapHandle);
        srvHeapHandle.Offset(1, m_cbvSrvUavDescriptorSize);
    }
}

void OnUpdate()
{
    PassConstant passConstants;
    XMVECTOR cameraPosition = XMVectorSet(-3.0f, 2.0f, -3.0f, 1.0f);
    XMVECTOR focusPosition = XMVectorZero();
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX viewMatrix = XMMatrixLookAtLH(cameraPosition, focusPosition, upDirection);
    XMStoreFloat3(&passConstants.cameraPositionInWorld, cameraPosition);
    XMStoreFloat4x4(&passConstants.viewMatrix, XMMatrixTranspose(viewMatrix));

    float fovAngleY = XM_PIDIV4;
    float aspectRatio = static_cast<float>(m_clientWidth) / m_clientHeight;
    float nearZ = 1.0f;
    float farZ = 20.0f;
    XMMATRIX projectMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
    XMStoreFloat4x4(&passConstants.projectMatrix, XMMatrixTranspose(projectMatrix));

    // ���ӹ�
    passConstants.ambientLight = { 0.3f, 0.3f, 0.3f, 1.0f };

    // ƽ�й�
    XMStoreFloat3(&passConstants.lights[0].direction, XMVector3Normalize(XMVectorSet(3.0f, -4.0f, -2.0f, 1.0f)));
    passConstants.lights[0].strength = { 1.0f, 1.0f, 1.0f };

    m_passConstantBuffer->CopyData(0, passConstants);
}

void OnRender()
{
    PopulateCommandList();

    // �ϴ�Command List��GPU
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // surface flipping
    ThrowIfFailed(m_swapChain->Present(1, 0));
    m_currendBackBufferIndex = (m_currendBackBufferIndex + 1) % m_swapChainBufferCount;

    FlushCommandQueue();
}

void PopulateCommandList()
{
    UINT passCount = 1;
    UINT itemCount = (UINT)m_renderItems.size();
    UINT materialCount = (UINT)m_materials.size();
    UINT textureCount = (UINT)m_textures.size();

    // Reset��һ֡ʹ�õ�Command Allocator�������洢��ǰ֡Ҫ��¼��Command
    ThrowIfFailed(m_commandAllocator->Reset());
    // Reset��һ֡ʹ�õ�Command List�����¼�¼��ǰ֡��Command
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    // Command List Reset����Ҫ��������
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[m_currendBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // ��ȡ��ǰ
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_currendBackBufferIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtvHandle, Colors::White, 0, nullptr);
    
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvHeap.Get() };
    int size = _countof(descriptorHeaps);
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, handle);   // pass��Ϣ

    // ����
    for (auto& item : m_renderItems)
    {
        m_commandList->IASetVertexBuffers(0, 1, &item->mesh->GetVertexBufferView());
        m_commandList->IASetIndexBuffer(&item->mesh->GetIndexBufferView());
        m_commandList->IASetPrimitiveTopology(item->primitiveType);

        // Object Constant Buffer
        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(item->objectCBIndex + passCount, m_cbvSrvUavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);

        // Material Constant Buffer
        cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(item->material->cbIndex + passCount + itemCount, m_cbvSrvUavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(2, cbvHandle);

        // Texture
        auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        srvHandle.Offset(item->material->albedoTextureIndex + passCount + itemCount + materialCount, m_cbvSrvUavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(3, srvHandle);

        m_commandList->DrawIndexedInstanced(item->indexCount, 1, item->startIndexLocation, item->baseVertexLocation, 0);
    }

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[m_currendBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_commandList->Close());
}

void OnDestroy()
{
    FlushCommandQueue();
    CloseHandle(m_fenceEvent);
}