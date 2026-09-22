#include "ImGuiLayer.h"

#include "Constants.h"
#include "Resource.h"
#include "context.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <cassert>

namespace
{
constexpr uint32_t kImGuiSrvCount = 16;
}

bool ImGuiLayer::Initialize(HWND window, Context& context)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = kImGuiSrvCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(context.DxDevice()->CreateDescriptorHeap(
            &heapDesc,
            IID_PPV_ARGS(&m_srvHeap))))
    {
        return false;
    }

    m_srvHeap->SetName(L"ImGui SRV Heap");
    m_descriptorSize = context.DxDevice()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_freeSrvIndices.reserve(kImGuiSrvCount);
    for (uint32_t index = kImGuiSrvCount; index > 0; --index)
    {
        m_freeSrvIndices.push_back(index - 1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(window))
    {
        ImGui::DestroyContext();
        return false;
    }

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = context.DxDevice();
    initInfo.CommandQueue = context.QueueDirect();
    initInfo.NumFramesInFlight = FRAME_BACKBUFFER_COUNT;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = m_srvHeap.Get();
    initInfo.SrvDescriptorAllocFn = &ImGuiLayer::AllocateSrv;
    initInfo.SrvDescriptorFreeFn = &ImGuiLayer::FreeSrv;
    initInfo.UserData = this;

    if (!ImGui_ImplDX12_Init(&initInfo))
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiLayer::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::DrawExampleWindow(float deltaTime)
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Begin("Renderer Debug");
    ImGui::Text("Frame time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Text("FPS: %.1f", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
    ImGui::Checkbox("Show ImGui demo", &m_showDemoWindow);
    ImGui::TextUnformatted("Scene -> Post Process -> ImGui -> Present");

    ImGui::SliderFloat("FrameBuffer Size",&(Context::GlobalSetting.RenderScale),0.1,2);
    ImGui::DragFloat3("Light Color",&(Context::GlobalSetting.MainLightColor.x));
    ImGui::DragFloat3("Light Dir",&(Context::GlobalSetting.MainLightDirection.x));
    ImGui::End();

    /*if (m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }*/
}

void ImGuiLayer::Render(Context& context)
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();

    CommandBuffer commandBuffer = context.GetCommandBufferPool()->AcquireCommandList(
        0,
        context.FrameIndex(),
        CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* commandList = commandBuffer.CmdList.Get();
    ID3D12Resource* backBuffer = context.GetBackBuffer(context.FrameIndex());

    D3D12_RESOURCE_BARRIER toRenderTarget{};
    toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRenderTarget.Transition.pResource = backBuffer;
    toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toRenderTarget);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = context.GetSwapChainView().cpuhandle;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    ID3D12DescriptorHeap* heaps[] = {m_srvHeap.Get()};
    commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    D3D12_RESOURCE_BARRIER toPresent{};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = backBuffer;
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toPresent);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* lists[] = {commandList};
        context.QueueDirect()->ExecuteCommandLists(1, lists);
    }
}

void ImGuiLayer::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_srvHeap.Reset();
    m_freeSrvIndices.clear();
    m_initialized = false;
}

void ImGuiLayer::AllocateSrv(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
{
    auto* layer = static_cast<ImGuiLayer*>(info->UserData);
    assert(layer && !layer->m_freeSrvIndices.empty());

    const uint32_t index = layer->m_freeSrvIndices.back();
    layer->m_freeSrvIndices.pop_back();

    *cpuHandle = layer->m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle->ptr += static_cast<SIZE_T>(index) * layer->m_descriptorSize;

    *gpuHandle = layer->m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle->ptr += static_cast<UINT64>(index) * layer->m_descriptorSize;
}

void ImGuiLayer::FreeSrv(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE)
{
    auto* layer = static_cast<ImGuiLayer*>(info->UserData);
    assert(layer);

    const SIZE_T begin = layer->m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    assert(cpuHandle.ptr >= begin);
    const SIZE_T byteOffset = cpuHandle.ptr - begin;
    assert(byteOffset % layer->m_descriptorSize == 0);

    const uint32_t index = static_cast<uint32_t>(byteOffset / layer->m_descriptorSize);
    assert(index < kImGuiSrvCount);
    layer->m_freeSrvIndices.push_back(index);
}
