#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class DX12Context {
public:
    bool Init(HWND hwnd, uint32_t width, uint32_t height);
    void Shutdown();
    static void EnableDebugLayer();  // 阶段3：必须在 Init 之前调用
    void Resize(uint32_t width, uint32_t height);  // 阶段3新增
    // ===== 阶段2新增：帧循环 =====
    ID3D12GraphicsCommandList* BeginFrame();  // 等待+重置，返回可录制的 cmdlist
    void EndFrame();                          // 关闭+提交+Present+Signal
    void WaitForGpu();                        // 等 GPU 全部完成（退出前用）
    
    ID3D12Device* GetDevice()  const { return m_device.Get(); }
    ID3D12CommandQueue* GetQueue()   const { return m_queue.Get(); }
    IDXGISwapChain3* GetSwapChain() const { return m_swapChain.Get(); }
    uint32_t              GetFrameIndex() const { return m_frameIndex; }
    ID3D12Resource* GetBackBuffer(uint32_t i) const { return m_backBuffers[i].Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const;
    static constexpr uint32_t kBackBufferCount = 2;
    uint32_t GetWidth()  const;// { return m_width; }
    uint32_t GetHeight() const;// { return m_height; }

    // ===== 阶段4新增：描述符分配 =====
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateDSV();
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateCBV_SRV_UAV();
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) const;
    ID3D12DescriptorHeap* GetSRVHeap() const { return m_srvHeap.Get(); }

    // ===== 阶段8新增：深度缓冲 =====
    void CreateDepthBuffer();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_dsvHandle; }
private:
    ComPtr<ID3D12Device>         m_device;
    ComPtr<ID3D12CommandQueue>   m_queue;
    ComPtr<IDXGISwapChain3>      m_swapChain;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12Resource>       m_backBuffers[kBackBufferCount];
    uint32_t                     m_rtvDescriptorSize = 0;
     
    // ===== 阶段2新增 =====
    ComPtr<ID3D12CommandAllocator>    m_cmdAllocators[kBackBufferCount]; // 每帧一个
    ComPtr<ID3D12GraphicsCommandList> m_cmdList;                          // 录制命令
    ComPtr<ID3D12Fence>               m_fence;                            // GPU→CPU 同步
    HANDLE   m_fenceEvent = nullptr;
    uint64_t m_nextFenceValue = 1;                  // 全局递增的 fence 值
    uint64_t m_frameFenceValues[kBackBufferCount] = {}; // 每个 buffer 最后一次的 fence 值
    uint32_t m_frameIndex = 0;
    // 阶段3新增
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool     m_initialized = false;
    // ===== 阶段4新增 =====
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;   // CBV/SRV/UAV 共享
    uint32_t m_dsvOffset = 0;
    uint32_t m_srvOffset = 0;
    uint32_t m_dsvDescriptorSize = 0;
    uint32_t m_srvDescriptorSize = 0;

    ComPtr<ID3D12Resource> m_depthBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle{};

};
