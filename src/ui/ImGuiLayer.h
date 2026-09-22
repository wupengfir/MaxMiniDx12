#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>
#include <vector>

class Context;

class ImGuiLayer
{
public:
    bool Initialize(HWND window, Context& context);
    void BeginFrame();
    void DrawExampleWindow(float deltaTime);
    void Render(Context& context);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

private:
    static void AllocateSrv(
        struct ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);
    static void FreeSrv(
        struct ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    std::vector<uint32_t> m_freeSrvIndices;
    uint32_t m_descriptorSize = 0;
    bool m_initialized = false;
    bool m_showDemoWindow = true;
};
