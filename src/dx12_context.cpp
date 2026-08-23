#include "dx12_context.h" 
#include <windows.h>

void DX12Context::EnableDebugLayer() {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
    }
}

bool DX12Context::Init(HWND hwnd, uint32_t width, uint32_t height) {
    m_width = width;   // 新增
    m_height = height; // 新增
    // 1. 创建 Device（Feature Level 11_0 足够，所有支持 DX12 的显卡都满足）
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
        return false;
    }

    // 2. 创建 Direct Command Queue（图形+计算+拷贝都能用这个队列）
    D3D12_COMMAND_QUEUE_DESC qDesc{};
    qDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(m_device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_queue)))) {
        return false;
    }

    // 3. 创建 SwapChain（FLIP_DISCARD，双缓冲）
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.BufferCount = kBackBufferCount;
    scDesc.Width       = width;
    scDesc.Height      = height;
    scDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(m_queue.Get(), hwnd, &scDesc, nullptr, nullptr, &swapChain1))) {
        return false;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER); // 禁用 Alt+Enter 全屏
    swapChain1.As(&m_swapChain); // 升级到 IDXGISwapChain3，为了 GetCurrentBackBufferIndex

    // 4. 创建 RTV Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = kBackBufferCount;
    rtvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTV 不需要 GPU 可见
    if (FAILED(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        return false;
    }
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 5. 为每个 BackBuffer 创建 RTV
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < kBackBufferCount; ++i) {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])))) {
            return false;
        }
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    // 6. Command Allocator：每个 backbuffer 一个，分配 GPU 命令内存
    for (uint32_t i = 0; i < kBackBufferCount; ++i) {
        if (FAILED(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocators[i])))) {
            return false;
        }
    }

    // 7. Command List：用第一个 allocator 创建，然后立即 Close（初始状态必须是 closed）
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_cmdAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_cmdList)))) {
        return false;
    }
    m_cmdList->Close();

    // 8. Fence + Win32 Event：GPU 完成后通过 Event 通知 CPU
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        return false;
    }
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) return false;

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 9. DSV Heap（深度缓冲用，不需要 GPU 可见）
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.NumDescriptors = 16;   // 留余量，后面 Shadow Map 也要用
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
        return false;
    }
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // 10. CBV/SRV/UAV Heap（必须 SHADER_VISIBLE，Shader 才能访问）
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 64;   // 常量缓冲+纹理+UAV 都从这里分
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_srvHeap)))) {
        return false;
    }
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 11. 深度缓冲
    CreateDepthBuffer();

    m_initialized = true;  // 新增
    return true;
}

void DX12Context::Shutdown() {
    WaitForGpu();  // 新增：确保 GPU 用完所有资源再释放
    if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }
    for (auto& a : m_cmdAllocators) a.Reset();
    m_cmdList.Reset();
    m_fence.Reset();
    for (auto& bb : m_backBuffers) bb.Reset();
    m_srvHeap.Reset();   // 新增
    m_dsvHeap.Reset();   // 新增
    m_rtvHeap.Reset();
    m_swapChain.Reset();
    m_queue.Reset();
    m_device.Reset();

    m_depthBuffer.Reset();
}


D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetBackBufferRTV(uint32_t index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += index * m_rtvDescriptorSize;
    return h;
}

ID3D12GraphicsCommandList* DX12Context::BeginFrame() {
    // 拿到当前要写的 backbuffer 下标
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 如果这个 backbuffer 上一帧的 GPU 命令还没执行完，CPU 在这里等
    uint64_t val = m_frameFenceValues[m_frameIndex];
    if (val > 0 && m_fence->GetCompletedValue() < val) {
        m_fence->SetEventOnCompletion(val, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    // 重置 allocator（清空命令内存）和 cmdlist（开始新的录制）
    m_cmdAllocators[m_frameIndex]->Reset();
    m_cmdList->Reset(m_cmdAllocators[m_frameIndex].Get(), nullptr);

    return m_cmdList.Get();
}

void DX12Context::EndFrame() {
    // 录制结束，关闭 cmdlist
    m_cmdList->Close();

    // 提交到 GPU 队列执行
    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_queue->ExecuteCommandLists(1, lists);

    // 翻转到屏幕（vsync on）
    m_swapChain->Present(1, 0);

    // GPU 执行完这个命令列表后，把 fence 值设为 val
    uint64_t val = m_nextFenceValue++;
    m_queue->Signal(m_fence.Get(), val);
    m_frameFenceValues[m_frameIndex] = val; // 记录这个 buffer 对应的 fence 值
}

void DX12Context::WaitForGpu() {
    // 提交一个空 Signal，等它完成 = 等之前所有命令完成
    uint64_t val = m_nextFenceValue++;
    m_queue->Signal(m_fence.Get(), val);
    if (m_fence->GetCompletedValue() < val) {
        m_fence->SetEventOnCompletion(val, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::GetCurrentBackBufferRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += m_frameIndex * m_rtvDescriptorSize;
    return h;
}

void DX12Context::Resize(uint32_t width, uint32_t height) {
    if (!m_initialized || width == 0 || height == 0) return;
    if (width == m_width && height == m_height) return;

    // 关键：等 GPU 执行完所有命令，否则释放 backbuffer 时 GPU 可能还在读
    WaitForGpu();

    // 释放旧 backbuffer（ResizeBuffers 要求没有对 buffer 的引用）
    for (auto& bb : m_backBuffers) bb.Reset();

    // 重建 SwapChain 的缓冲
    DXGI_SWAP_CHAIN_DESC desc{};
    m_swapChain->GetDesc(&desc);
    m_swapChain->ResizeBuffers(kBackBufferCount, width, height,
        desc.BufferDesc.Format, desc.Flags);

    m_width = width;
    m_height = height;
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 重新为每个 backbuffer 创建 RTV
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < kBackBufferCount; ++i) {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    // 窗口尺寸变了，深度缓冲也要重建
    CreateDepthBuffer();
}


uint32_t DX12Context::GetWidth()  const { return m_width; }
uint32_t DX12Context::GetHeight() const { return m_height; }

D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::AllocateDSV() {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += m_dsvOffset * m_dsvDescriptorSize;
    m_dsvOffset++;
    return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Context::AllocateCBV_SRV_UAV() {
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += m_srvOffset * m_srvDescriptorSize;
    m_srvOffset++;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Context::GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) const {
    // 通过 CPU 句柄算出在 Heap 中的偏移，再映射到 GPU 句柄
    UINT offset = (UINT)((cpuHandle.ptr - m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr)
        / m_srvDescriptorSize);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    gpu.ptr += (SIZE_T)offset * m_srvDescriptorSize;
    return gpu;
}

void DX12Context::CreateDepthBuffer() {
    // 释放旧的（Resize 时会调用）
    m_depthBuffer.Reset();

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeap.CreationNodeMask = 1;
    defaultHeap.VisibleNodeMask = 1;

    // 深度缓冲是一张 2D 纹理，必须带 ALLOW_DEPTH_STENCIL flag
    D3D12_RESOURCE_DESC dsDesc{};
    dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Width = m_width;
    dsDesc.Height = m_height;
    dsDesc.DepthOrArraySize = 1;
    dsDesc.MipLevels = 1;
    dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 创建时指定 CLEAR_VALUE 和初始状态 DEPTH_WRITE
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;  // 清深度为最远

    m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&m_depthBuffer));

    // 从 DSV Heap 分配一个描述符，创建 DSV
    m_dsvHandle = AllocateDSV();
    m_device->CreateDepthStencilView(m_depthBuffer.Get(), nullptr, m_dsvHandle);

    m_depthBuffer->SetName(L"Depth Buffer");
}
