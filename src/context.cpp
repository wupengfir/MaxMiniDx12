#include "context.h"
#include "mesh.h"
#include "Material.h"
GraphicsDevice::GraphicsDevice(bool useDebugLayer):useDebug(useDebugLayer)
{
    
}

void GraphicsDevice::SyncGPU(UINT64 signal)
{
	if (signal > 0 && m_fence->GetCompletedValue() < signal) {
        m_fence->SetEventOnCompletion(signal, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void GraphicsDevice::WaitForGpu() {
    // 提交一个空 Signal，等它完成 = 等之前所有命令完成
    uint64_t val = m_nextFenceValue++;
    m_queueDirect->Signal(m_fence.Get(), val);
    SyncGPU(val);
}

void GraphicsDevice::BeginFrame() {
    // 拿到当前要写的 backbuffer 下标
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 如果这个 backbuffer 上一帧的 GPU 命令还没执行完，CPU 在这里等
    uint64_t val = m_frameFenceValues[m_frameIndex];
    SyncGPU(val);

    m_commandBufferPool->ResetAllocators(0,m_frameIndex);

}

void GraphicsDevice::EndFrame() {
    //// 录制结束，关闭 cmdlist
    //m_cmdList->Close();

    //// 提交到 GPU 队列执行
    //ID3D12CommandList* lists[] = { m_cmdList.Get() };
    //m_queue->ExecuteCommandLists(1, lists);

    // 翻转到屏幕（vsync on）
    m_swapChain->Present(1, 0);

    // GPU 执行完这个命令列表后，把 fence 值设为 val
    uint64_t val = m_nextFenceValue++;
    m_queueDirect->Signal(m_fence.Get(), val);
    m_frameFenceValues[m_frameIndex] = val; // 记录这个 buffer 对应的 fence 值
}

void GraphicsDevice::DrawMesh(ID3D12GraphicsCommandList* cmdList,MyMesh* mesh,Material* mat)
{
    /*ID3D12PipelineState* pso;
    auto it = PSOManager::PsoMap.find(mat);
    if (it != PSOManager::PsoMap.end())
    {
        pso = it->second;
    }*/

    ID3D12PipelineState* pso = mat->GetPso().Get();
    if (pso)
    {

    }
    else
    {
        mat->PsoDesc().InputLayout = { mesh->GetInputDesc().data(),(UINT)mesh->GetInputDesc().size() };
        DxDevice()->CreateGraphicsPipelineState(&mat->PsoDesc(), IID_PPV_ARGS(&(mat->GetPso())));
    }

    cmdList->SetGraphicsRootSignature(mat->GetRootSignature().Get());
    cmdList->SetPipelineState(mat->GetPso().Get());

    cmdList->IASetVertexBuffers(0, 1, mesh->VBV());
    if (mesh->indices.size() > 0)
    {
        cmdList->IASetIndexBuffer(mesh->IBV());
        cmdList->DrawIndexedInstanced(mesh->indices.size(),1,0,0,0);
    }
    else
    {
        cmdList->DrawInstanced(mesh->vertices.size(), 1, 0, 0);
    }
}

bool GraphicsDevice::Init(HWND hwnd, uint32_t width, uint32_t height)
{
    ScreenWidth = width;
	ScreenHeight = height;
	if (useDebug)
	{
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug)))) 
		{
			m_debug->EnableDebugLayer();
		}
	}

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_factory));
	if (FAILED(hr))
	{
		return false;
	}
	m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER); // 禁用 Alt+Enter 全屏
	for(UINT i = 0; ; i++)
	{
		// DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE：高性能独显
		HRESULT hr = m_factory->EnumAdapters1(
			i,
			&m_adapter);

		if(hr == DXGI_ERROR_NOT_FOUND)
			break; //枚举结束

		if(FAILED(hr)) continue;

		// 跳过软件适配器，只拿硬件GPU
		DXGI_ADAPTER_DESC1 desc{};
        if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}
		m_adapter->GetDesc1(&desc);
        break;
	}

	if (FAILED(D3D12CreateDevice(Adapter(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
	{
		return false;
	}
    pDefaultDevice = this;
    Resource::m_device = this;
    m_commandBufferPool = new CommandBufferPool(1);
    m_DsvHeap = new DSVDescriptorHeap(4);
    m_RtvHeap = new RTVDescriptorHeap(16);

	
	D3D12_COMMAND_QUEUE_DESC qDesc{};
    qDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(DxDevice()->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_queueDirect)))) {
        return false;
    }
	qDesc.Type  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    if (FAILED(DxDevice()->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_queueCompute)))) {
        return false;
    }
	qDesc.Type  = D3D12_COMMAND_LIST_TYPE_COPY;
    if (FAILED(DxDevice()->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_queueCopy)))) {
        return false;
    }


	DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.BufferCount = FRAME_BACKBUFFER_COUNT;
    scDesc.Width       = width;
    scDesc.Height      = height;
    scDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(m_factory->CreateSwapChainForHwnd(QueueDirect(), hwnd, &scDesc, nullptr, nullptr, &swapChain1))) {
        return false;
    }
	
    swapChain1.As(&m_swapChain); // 升级到 IDXGISwapChain3，为了 GetCurrentBackBufferIndex
    for (uint32_t i = 0; i < FRAME_BACKBUFFER_COUNT; ++i) {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])))) {
            return false;
        }
        D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		DescriptorHeap::ViewDesc viewdesc(&rtDesc);
        m_RtvHeap->CreateView(m_backBuffers[i].Get(),viewdesc);
    }

    // 8. Fence + Win32 Event：GPU 完成后通过 Event 通知 CPU
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        return false;
    }
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) return false;

}