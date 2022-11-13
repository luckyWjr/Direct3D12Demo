#include <windows.h>
#include "../Common/d3d12Util.h"
#include "../Common/UploadHeapConstantBuffer.h"
#include <unordered_map> 

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// 顶点数据
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
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

    // 模型变换矩阵，即物体在世界坐标下的位置，缩放以及旋转值
    XMFLOAT4X4 modelMatrix = MathUtil::Identity4x4();
    // 对应ObjectConstant的下标
    UINT objectCBIndex = -1;

    // 对应的vertex和index buffer信息
    Mesh* mesh = nullptr;

    // 对应的材质
    Material* material = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // 对应mesh中的Submesh值
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    int baseVertexLocation = 0;
};


UINT m_clientWidth = 800, m_clientHeight = 600;     // 窗口尺寸
HWND m_hwnd;
UINT64 m_fenceValue;
static const UINT m_swapChainBufferCount = 2;
HANDLE m_fenceEvent;
UINT m_rtvDescriptorSize, m_cbvSrvUavDescriptorSize;
UINT m_currendBackBufferIndex = 0;

DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;  // 每个元素占用8-bit，值的范围[0,1]

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
std::vector<std::unique_ptr<RenderItem>> m_renderItems;
std::unique_ptr<UploadHeapConstantBuffer<ObjectConstant>> m_objectConstantBuffer;
std::unique_ptr<UploadHeapConstantBuffer<PassConstant>> m_passConstantBuffer;
std::unique_ptr<UploadHeapConstantBuffer<MaterialConstant>> m_materialConstantBuffer;
ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
ComPtr<ID3DBlob> m_vsByteCode;
ComPtr<ID3DBlob> m_psByteCode;
ComPtr<ID3D12PipelineState> m_pipelineState;

D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissorRect;

std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitDirect3D();
void InitAsset();
void InitMeshes();
void InitRenderItems();
void InitMaterials();
void InitConstantBuffer();
void FlushCommandQueue();
void CreateRootSignature();
void PopulateCommandList();
void OnUpdate();
void OnRender();
void OnDestroy();


// 应用程序入口函数
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
        L"Local Lit",                   // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // 窗口大小
        m_clientWidth, m_clientHeight,  // 窗口位置
        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (m_hwnd == NULL) return 0;

    ShowWindow(m_hwnd, nCmdShow);         // 显示窗口

    try
    {
        InitDirect3D();
        InitAsset();

        // 获取事件，例如鼠标点击，键盘按键，窗口变化等等。
        MSG msg = { };
        while (msg.message != WM_QUIT)      // message loop
        {
            // PeekMessage 检查 message queue中是否有message，如果有的话将其到msg中，返回非0的值
            // PM_REMOVE 除了WM_PAINT类型的message外，其他的message会在执行完PeekMessage后被移出 message queue
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);      // 会触发 WindowProc 函数
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

// 事件处理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);     // 中止Message loop
        return 0;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
    // 开启 D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    // 禁止使用alt+enter切换全屏模式
    ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_PRINT_SCREEN));

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
    {
        // 如果device创建失败，Fallback 到 WARP device.
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Command Queue，Command Allocator，Command List的type要一致
    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = commandListType;                   // Command List的类型
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;    // 默认的Command Queue
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
    sd.SampleDesc.Count = 1;                                // 不能设置multisample
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
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));  // 会导致Swap Chain里的buffer对象引用计数+1，由于使用了ComPtr，会自动调用release
        m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);       // 偏移一个rtv大小
    }
}

void InitAsset()
{
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_INPUT_LAYOUT_DESC inputLayout = { inputElementDescs, _countof(inputElementDescs) };

    m_commandList->Reset(m_commandAllocator.Get(), nullptr); // 之前close了command list，这里要使用到，需要reset操作才可记录command。

    InitMeshes();
    InitMaterials();
    InitRenderItems();
    InitConstantBuffer();
    CreateRootSignature();

    // shader compiler
    m_vsByteCode = d3d12Util::LoadBinary(L"shaders_vs.cso");
    m_psByteCode = d3d12Util::LoadBinary(L"shaders_ps.cso");

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayout;
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vsByteCode.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_psByteCode.Get());
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

    // 窗口大小
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = static_cast<float>(m_clientWidth);
    m_viewport.Height = static_cast<float>(m_clientHeight);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    // 裁剪区域
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
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));             // 添加一个指令到GPU，用来更新fence值

    if (m_fence->GetCompletedValue() < m_fenceValue)                                // 获取当前的fence值
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));   // 当fence值等于m_fenceValue时，会触发m_fenceEvent
        WaitForSingleObject(m_fenceEvent, INFINITE);                                // 等待m_fenceEvent被执行
    }
}

void CreateRootSignature()
{
    // 一个root signature定义一个root parameter数组
    // 一个root parameter可以是root constant，root descriptor，descriptor table
    CD3DX12_ROOT_PARAMETER rootParameters[3];
    CD3DX12_DESCRIPTOR_RANGE passCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);       // register(b0)，后续绑定Pass Constant Buffer
    CD3DX12_DESCRIPTOR_RANGE objectCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);     // register(b1)，后续绑定Object Constant Buffer
    CD3DX12_DESCRIPTOR_RANGE materialCbvTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);   // register(b2)，后续绑定Material Constant Buffer
    rootParameters[0].InitAsDescriptorTable(1, &passCbvTable);
    rootParameters[1].InitAsDescriptorTable(1, &objectCbvTable);
    rootParameters[2].InitAsDescriptorTable(1, &materialCbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void InitMeshes()
{
    // 几种几何体的顶点和下标信息
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;

    GeometryManager::MeshData cubeMeshData = GeometryManager::CreateCube(1);
    GeometryManager::MeshData pyramidMeshData = GeometryManager::CreatePyramid(1, 2);
    GeometryManager::MeshData sphereMeshData = GeometryManager::CreateSphere(1, 15, 30);

    for (int i = 0; i < cubeMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ cubeMeshData.vertices[i].position, cubeMeshData.vertices[i].normal }));
    for (int i = 0; i < pyramidMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ pyramidMeshData.vertices[i].position, pyramidMeshData.vertices[i].normal }));
    for (int i = 0; i < sphereMeshData.vertices.size(); ++i)
        vertices.push_back(Vertex({ sphereMeshData.vertices[i].position, sphereMeshData.vertices[i].normal }));

    indices.insert(indices.end(), std::begin(cubeMeshData.indices), std::end(cubeMeshData.indices));
    indices.insert(indices.end(), std::begin(pyramidMeshData.indices), std::end(pyramidMeshData.indices));
    indices.insert(indices.end(), std::begin(sphereMeshData.indices), std::end(sphereMeshData.indices));

    const UINT verticesSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indicesSize = (UINT)indices.size() * sizeof(std::uint16_t);

    // 单个几何体在buffer中的位置
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

    // 创建mesh对象
    auto mesh = std::make_unique<Mesh>();
    mesh->name = "baseGeometryMesh";

    // 将vertices和indices数据拷贝到mesh中
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

void InitMaterials()
{
    auto cube = std::make_unique<Material>();
    cube->name = "cube";
    cube->cbIndex = 0;
    cube->albedo = XMFLOAT4(Colors::LightBlue);
    cube->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    cube->roughness = 0.1f;

    auto pyramid = std::make_unique<Material>();
    pyramid->name = "pyramid";
    pyramid->cbIndex = 1;
    pyramid->albedo = XMFLOAT4(Colors::Yellow);
    pyramid->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    pyramid->roughness = 0.2f;

    auto sphere = std::make_unique<Material>();
    sphere->name = "sphere";
    sphere->cbIndex = 2;
    sphere->albedo = XMFLOAT4(Colors::Orange);
    sphere->fresnelR0 = XMFLOAT3(0.08f, 0.08f, 0.08f);
    sphere->roughness = 0.3f;

    m_materials["cube"] = std::move(cube);
    m_materials["pyramid"] = std::move(pyramid);
    m_materials["sphere"] = std::move(sphere);
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
    cubeItem->material = m_materials["cube"].get();
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
    pyramidItem->material = m_materials["pyramid"].get();
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
    sphereItem->material = m_materials["sphere"].get();
    sphereItem->indexCount = sphereItem->mesh->drawArgs["sphere"].indexCount;
    sphereItem->startIndexLocation = sphereItem->mesh->drawArgs["sphere"].startIndexLocation;
    sphereItem->baseVertexLocation = sphereItem->mesh->drawArgs["sphere"].baseVertexLocation;
    m_renderItems.push_back(std::move(sphereItem));

    m_objectConstantBuffer = std::make_unique<UploadHeapConstantBuffer<ObjectConstant>>(m_device.Get(), m_renderItems.size());
    // 设置Object Constant Buffer
    for (auto& item : m_renderItems)
    {
        XMMATRIX world = XMLoadFloat4x4(&item->modelMatrix);
        ObjectConstant objectConstant;
        XMStoreFloat4x4(&objectConstant.modelMatrix, XMMatrixTranspose(world));
        m_objectConstantBuffer->CopyData(item->objectCBIndex, objectConstant);
    }
}

void InitConstantBuffer()
{
    // 创建Pass Constant Buffer，在update里设置值
    m_passConstantBuffer = std::make_unique<UploadHeapConstantBuffer<PassConstant>>(m_device.Get(), 1);

    // 创建、设置Object Constant Buffer
    m_objectConstantBuffer = std::make_unique<UploadHeapConstantBuffer<ObjectConstant>>(m_device.Get(), m_renderItems.size());
    for (auto& item : m_renderItems)
    {
        XMMATRIX world = XMLoadFloat4x4(&item->modelMatrix);
        ObjectConstant objectConstant;
        XMStoreFloat4x4(&objectConstant.modelMatrix, XMMatrixTranspose(world));
        m_objectConstantBuffer->CopyData(item->objectCBIndex, objectConstant);
    }

    // 创建、设置Material Constant Buffer
    m_materialConstantBuffer = std::make_unique<UploadHeapConstantBuffer<MaterialConstant>>(m_device.Get(), m_materials.size());
    for (auto& e : m_materials)
    {
        Material* mat = e.second.get();
        MaterialConstant materialConstant;
        materialConstant.albedo = mat->albedo;
        materialConstant.fresnelR0 = mat->fresnelR0;
        materialConstant.roughness = mat->roughness;
        m_materialConstantBuffer->CopyData(mat->cbIndex, materialConstant);
    }

    UINT itemCount = (UINT)m_renderItems.size();
    UINT materialCount = (UINT)m_materials.size();

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1 + itemCount + materialCount;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHeapHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
    m_passConstantBuffer->CreateConstantBufferView(m_device.Get(), cbvHeapHandle, 0);

    // Object Constant Buffer对应的CBV
    for (UINT i = 0; i < itemCount; ++i)
    {
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(i + 1, m_cbvSrvUavDescriptorSize);
        m_objectConstantBuffer->CreateConstantBufferView(m_device.Get(), handle, i);
    }

    // Material Constant Buffer对应的CBV
    for (UINT i = 0; i < materialCount; ++i)
    {
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(i + 1 + itemCount, m_cbvSrvUavDescriptorSize);
        m_materialConstantBuffer->CreateConstantBufferView(m_device.Get(), handle, i);
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

    // 设间接光
    passConstants.ambientLight = { 0.3f, 0.3f, 0.3f, 1.0f };

    // 平行光
    XMStoreFloat3(&passConstants.lights[0].direction, XMVector3Normalize(XMVectorSet(3.0f, -4.0f, -2.0f, 1.0f)));
    passConstants.lights[0].strength = { 1.0f, 1.0f, 1.0f };

    // 点光源
    passConstants.lights[1].strength = { 1.0f, 0.0f, 0.0f };
    passConstants.lights[1].falloffStart = 0;
    passConstants.lights[1].falloffEnd = 15;
    passConstants.lights[1].Position = { 3.0f, 1.5f, -1.5f };

    // 聚光灯
    passConstants.lights[2].strength = { 0.0f, 1.0f, 0.0f };
    passConstants.lights[2].falloffStart = 0;
    passConstants.lights[2].falloffEnd = 40;
    passConstants.lights[2].Position = { -3.0f, 2.0f, -5.0f };
    XMStoreFloat3(&passConstants.lights[2].direction, XMVector3Normalize(XMVectorSet(1.0f, -1.0f, 1.0f, 1.0f)));
    passConstants.lights[2].spotPower = 16;

    m_passConstantBuffer->CopyData(0, passConstants);
}

void OnRender()
{
    PopulateCommandList();

    // 上传Command List到GPU
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // surface flipping
    ThrowIfFailed(m_swapChain->Present(1, 0));
    m_currendBackBufferIndex = (m_currendBackBufferIndex + 1) % m_swapChainBufferCount;

    FlushCommandQueue();
}

void PopulateCommandList()
{
    // Reset上一帧使用的Command Allocator，用来存储当前帧要记录的Command
    ThrowIfFailed(m_commandAllocator->Reset());
    // Reset上一帧使用的Command List，重新记录当前帧的Command
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    // Command List Reset后，需要重新设置
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[m_currendBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 获取当前
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_currendBackBufferIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtvHandle, Colors::White, 0, nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, handle);   // pass信息

    // 绘制
    for (auto& item : m_renderItems)
    {
        m_commandList->IASetVertexBuffers(0, 1, &item->mesh->GetVertexBufferView());
        m_commandList->IASetIndexBuffer(&item->mesh->GetIndexBufferView());
        m_commandList->IASetPrimitiveTopology(item->primitiveType);

        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(item->objectCBIndex + 1, m_cbvSrvUavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);

        cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(item->material->cbIndex + 4, m_cbvSrvUavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(2, cbvHandle);

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