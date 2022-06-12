#include <windows.h>
#include "../Common/d3d12Util.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

UINT m_clientWidth = 800, m_clientHeight = 600;     // ���ڳߴ�
HWND m_hwnd;
UINT64 m_fenceValue;
static const UINT m_swapChainBufferCount = 2;
HANDLE m_fenceEvent;
UINT m_rtvDescriptorSize;
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
ComPtr<ID3D12Resource> m_vertexBuffer;
ComPtr<ID3DBlob> m_vsByteCode;
ComPtr<ID3DBlob> m_psByteCode;
ComPtr<ID3D12PipelineState> m_pipelineState;

D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissorRect;

// ��������
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct ObjectConstants
{
    XMFLOAT4X4 ModelMatrix = MathUtil::Identity4x4();
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitDirect3D();
void InitAsset();
void FlushCommandQueue();
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
        L"Learn to Program Windows",    // Window text
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

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < m_swapChainBufferCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));  // �ᵼ��Swap Chain���buffer�������ü���+1������ʹ����ComPtr�����Զ�����release
        m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);       // ƫ��һ��rtv��С
    }
}

double m_secondsPerCount;

void InitAsset()
{
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    m_secondsPerCount = 1.0 / (double)countsPerSec;

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_INPUT_LAYOUT_DESC inputLayout = { inputElementDescs, _countof(inputElementDescs) };

    // ע�ⶥ��˳����ʱ������Ϳ�������
    Vertex triangleVertices[] =
    {        
        { { 0.0f, 0.5f, 0.0f }, XMFLOAT4(Colors::Red) },
        { { 0.5f, -0.5f, 0.0f }, XMFLOAT4(Colors::Green)},
        { { -0.5f, -0.5f, 0.0f }, XMFLOAT4(Colors::Blue) },
    };

    // ȷ���������ݲ�����Command��ִ�к󣬿����ͷ�upload heap��resource��ͨ��ComPtr���Զ�Release
    ComPtr<ID3D12Resource> vertexUploadBuffer;

    const UINT triangleVerticesSize = sizeof(triangleVertices);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(triangleVerticesSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)));

    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(triangleVerticesSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexUploadBuffer)));

    // ����Ҫ�����CPU����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = triangleVertices;
    subResourceData.RowPitch = triangleVerticesSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    m_commandList->Reset(m_commandAllocator.Get(), nullptr); // ֮ǰclose��command list������Ҫʹ�õ�����Ҫreset�����ſɼ�¼command��

    // ����ǰҪ����resource��state
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    // ���Ƚ�CPU�ڴ����ݿ�����upload heap�У�Ȼ����ͨ��ID3D12CommandList::CopySubresourceRegion��upload heap�п�����default buffer��
    UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer.Get(), vertexUploadBuffer.Get(), 0, 0, 1, &subResourceData);
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = triangleVerticesSize;
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);

    // ����һ���յ�root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

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
}

void FlushCommandQueue()
{
    m_fenceValue++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));             // ���һ��ָ�GPU����������fenceֵ

    if (m_fence->GetCompletedValue() < m_fenceValue)                                // ��ȡ��ǰ��fenceֵ
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));   // ��fenceֵ����m_fenceValueʱ���ᴥ��m_fenceEvent
        WaitForSingleObject(m_fenceEvent, INFINITE);                                // �ȴ�m_fenceEvent��ִ��
    }
}

void PopulateCommandList()
{
    // Reset��һ֡ʹ�õ�Command Allocator�������洢��ǰ֡Ҫ��¼��Command
    ThrowIfFailed(m_commandAllocator->Reset());
    // Reset��һ֡ʹ�õ�Command List�����¼�¼��ǰ֡��Command
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // Command List Reset����Ҫ��������
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[m_currendBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // ��ȡ��ǰ
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_currendBackBufferIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtvHandle, Colors::Gray, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[m_currendBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_commandList->Close());
}

void OnUpdate()
{
}

int i = 0;

void OnRender()
{
    PopulateCommandList();

    // �ϴ�Command List��GPU
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


    DXGI_FRAME_STATISTICS sta;
    m_swapChain->GetFrameStatistics(&sta);
    i = i % 4;
    std::wstring text = L"before Present = " + std::to_wstring(i + 1) + L"  PresentCount = " + std::to_wstring(sta.PresentCount) + L"  PresentRefreshCount = " + std::to_wstring(sta.PresentRefreshCount) + L"\n";
    OutputDebugString(text.c_str());
    __int64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

    // surface flipping
    ThrowIfFailed(m_swapChain->Present(1, 0));
    m_currendBackBufferIndex = (m_currendBackBufferIndex + 1) % m_swapChainBufferCount;

    FlushCommandQueue();

    __int64 endTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
    std::wstring timetext = L"Time = " + std::to_wstring((endTime - startTime) * m_secondsPerCount) + L"\n";
    OutputDebugString(timetext.c_str());
}

void OnDestroy()
{
    FlushCommandQueue();
    CloseHandle(m_fenceEvent);
}