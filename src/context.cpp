#include "context.h"
#include "mesh.h"
#include "Material.h"
#include "Camera.h"
Context::Context(bool useDebugLayer):useDebug(useDebugLayer)
{
    
}

void Context::SetRenderTarget(ID3D12GraphicsCommandList* cmdList,TextureBuffer* rt,bool clear,DXGI_RGBA clearColor,TextureBuffer* depth,bool clearDepth,float clearDepthValue)
{
    ID3D12Resource* colorResource = rt->GetTexture();
    auto colorstatus = Resource::StatusMap.find(colorResource);
    if (colorstatus == Resource::StatusMap.end())
    {
        return;
    }
    if (colorstatus->second.status != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        // 1. BackBuffer 从 PRESENT 状态过渡到 RENDER_TARGET（才能写）
        D3D12_RESOURCE_BARRIER toRender{};
        toRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRender.Transition.pResource = colorResource;
        //toRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRender.Transition.StateBefore = colorstatus->second.status;
        toRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &toRender);
        colorstatus->second.status = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
	        

    // 清屏
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = ((TextureBuffer*)Resource::FindResourceAndStatus(colorResource).resource)->CPUHandles[(int)ViewType::RTV];// m_Color->cpuhandle;
    cmdList->ClearRenderTargetView(rtv, &clearColor.r, 0, nullptr);            
    if (depth)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = ((TextureBuffer*)Resource::FindResourceAndStatus(depth->GetTexture()).resource)->CPUHandles[(int)ViewType::DSV];// m_Depth->cpuhandle;   
        cmdList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    }
    else
    {
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    }
    

    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = colorResource->GetDesc().Width;
    viewport.Height = colorResource->GetDesc().Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor{};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = colorResource->GetDesc().Width;
    scissor.bottom = colorResource->GetDesc().Height;
    cmdList->RSSetScissorRects(1, &scissor);
}

void Context::SyncGPU(UINT64 signal)
{
	if (signal > 0 && m_fence->GetCompletedValue() < signal) {
        m_fence->SetEventOnCompletion(signal, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void Context::WaitForGpu() {
    // 提交一个空 Signal，等它完成 = 等之前所有命令完成
    uint64_t val = m_nextFenceValue++;
    m_queueDirect->Signal(m_fence.Get(), val);
    SyncGPU(val);
}

void Context::CreateFrameBuffers()
{
    depthbuffer.Width = ScreenWidth * Context::GlobalSetting.RenderScale;
    depthbuffer.Height = ScreenHeight * Context::GlobalSetting.RenderScale;
    depthbuffer.CreateTexture();

    colorbuffer.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    colorbuffer.Width = ScreenWidth * Context::GlobalSetting.RenderScale;
    colorbuffer.Height = ScreenHeight * Context::GlobalSetting.RenderScale;
    colorbuffer.CreateTexture();
}

void Context::ResizeFrameBuffers()
{
    if (depthbuffer.GetTexture())
    {
        depthbuffer.Release();
    }
    depthbuffer.Width = ScreenWidth * Context::GlobalSetting.RenderScale;
    depthbuffer.Height = ScreenHeight * Context::GlobalSetting.RenderScale;
    depthbuffer.CreateTexture();
    if (colorbuffer.GetTexture())
    {
        colorbuffer.Release();
    }
    colorbuffer.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    colorbuffer.Width = ScreenWidth * Context::GlobalSetting.RenderScale;
    colorbuffer.Height = ScreenHeight * Context::GlobalSetting.RenderScale;
    colorbuffer.CreateTexture();
}

void Context::Update()
{
    if (GlobalSetting.RenderScale != GlobalSetting.PrevRenderScale)
    {
        WaitForGpu();
        ResizeFrameBuffers();
        GlobalSetting.PrevRenderScale = GlobalSetting.RenderScale;
    }
}

void Context::BeginFrame() {

    FrameCount++;
	timer.CountTime();

    // 拿到当前要写的 backbuffer 下标
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 如果这个 backbuffer 上一帧的 GPU 命令还没执行完，CPU 在这里等
    uint64_t val = m_frameFenceValues[m_frameIndex];
    SyncGPU(val);

    m_commandBufferPool->ResetAllocators(0,m_frameIndex);

}

void Context::EndFrame() {
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

void Context::DrawMesh(ID3D12GraphicsCommandList* cmdList,Mesh* mesh,int submeshIndex,Material* mat,const XMMATRIX* matrix ,Camera* camera,DXGI_FORMAT formats[],const XMMATRIX* matrixInvs )
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
        mat->Create();
        mat->PsoDesc().InputLayout = { mesh->GetInputDesc().data(),(UINT)mesh->GetInputDesc().size() };
        memcpy(mat->PsoDesc().RTVFormats, formats, sizeof(DXGI_FORMAT) * 8);
        DxDevice()->CreateGraphicsPipelineState(&mat->PsoDesc(), IID_PPV_ARGS(&(mat->GetPso())));
    }

    cmdList->SetGraphicsRootSignature(mat->GetRootSignature().Get());
    cmdList->SetPipelineState(mat->GetPso().Get());

    //设置材质属性
    auto& paramList = mat->GetShader()->GetShaderResourceParams();
    UINT slotIndex = 0;
    for (size_t i = 0; i < paramList.size(); i++)
    {
        auto& param = paramList[i];
        //判断cb
        if (param.bufferDataDescs.size() > 0)
        {
            //perframe buffer
            if (param.bindDesc.BindPoint == 0 && param.bindDesc.Space == 0)
            {
                
                if (param.AlignedConstantBuffer == nullptr)
                param.AlignedConstantBuffer = new uint8_t[param.alignedCBufferSize]{};
                auto value = param.bufferDataDescs[0];
                XMFLOAT3 cameraPos = camera->GetTransform().GetPos();
                XMFLOAT4 cameraPos4 = XMFLOAT4(cameraPos.x,cameraPos.y,cameraPos.z,1);
                memcpy(param.AlignedConstantBuffer+value.StartOffset,&cameraPos4,16);
                value = param.bufferDataDescs[1];
                XMMATRIX vp = camera->ViewMatrix() * camera->ProjectionMatrix();
                memcpy(param.AlignedConstantBuffer+value.StartOffset,&vp,64);
                /*XMVECTOR tt = XMVector4Transform(XMVECTOR{ 0,0,0,1 }, vp);*/
                auto offset = CBufferHeap()->WriteConstantBuffer(param.AlignedConstantBuffer,param.alignedCBufferSize);
                cmdList->SetGraphicsRootConstantBufferView(slotIndex,CBufferHeap()->GetAddress() + offset);
                slotIndex++;
            }
             //perdraw buffer
            else if (param.bindDesc.BindPoint == 1 && param.bindDesc.Space == 0)
            {
                if (param.AlignedConstantBuffer == nullptr)
                param.AlignedConstantBuffer = new uint8_t[param.alignedCBufferSize]{};

                auto value = param.bufferDataDescs[0]; 
                memcpy(param.AlignedConstantBuffer+value.StartOffset,matrix,value.Size);
                value = param.bufferDataDescs[1]; 
                if (matrixInvs)
                {
                    memcpy(param.AlignedConstantBuffer+value.StartOffset,matrixInvs,value.Size);
                }
                else
                {
                    memset(param.AlignedConstantBuffer+value.StartOffset,0,value.Size);
                }                

                auto offset = CBufferHeap()->WriteConstantBuffer(param.AlignedConstantBuffer,param.alignedCBufferSize);
                cmdList->SetGraphicsRootConstantBufferView(slotIndex,CBufferHeap()->GetAddress() + offset);
                slotIndex++;
            }
            //perFrame buffer
            else if (param.bindDesc.BindPoint == 2 && param.bindDesc.Space == 0)
            {
                if (param.AlignedConstantBuffer == nullptr)
                param.AlignedConstantBuffer = new uint8_t[param.alignedCBufferSize]{};

                auto value = param.bufferDataDescs[0]; 
                auto dir = XMVector3Normalize(XMVECTOR{ GlobalSetting.MainLightDirection.x,GlobalSetting.MainLightDirection.y,GlobalSetting.MainLightDirection.z,GlobalSetting.MainLightDirection.w });
                memcpy(param.AlignedConstantBuffer+value.StartOffset,&dir,value.Size);
                value = param.bufferDataDescs[1]; 
                memcpy(param.AlignedConstantBuffer+value.StartOffset,&GlobalSetting.MainLightColor,value.Size);
                value = param.bufferDataDescs[2];
				float deltaTime = Context::pContext->timer.DeltaTime(); 
                XMVECTOR time = { deltaTime ,sin(deltaTime),Context::pContext->timer.ElapsedSeconds(),Context::pContext->FrameCount};
                memcpy(param.AlignedConstantBuffer + value.StartOffset, &time, value.Size);
                auto offset = CBufferHeap()->WriteConstantBuffer(param.AlignedConstantBuffer,param.alignedCBufferSize);
                cmdList->SetGraphicsRootConstantBufferView(slotIndex,CBufferHeap()->GetAddress() + offset);
                slotIndex++;
            }
           

            else
            {
                for (size_t j = 0; j < param.bufferDataDescs.size(); j++)
                {
                    if (param.AlignedConstantBuffer == nullptr)
                    param.AlignedConstantBuffer = new uint8_t[param.alignedCBufferSize]{};
                    auto value = param.bufferDataDescs[j];
                    auto name = param.bufferDataDescNames[j];
                    MaterialProperty* property = mat->FindProperty(name);
                    if (property)
                    {
                        //printf("x is :  %f\n",*((float*)(property->data)));
                        memcpy(param.AlignedConstantBuffer+value.StartOffset,property->data,property->size);
                    }
                    else
                    {
                        memset(param.AlignedConstantBuffer+value.StartOffset,0,value.Size);
                    }
                
                }           
                auto offset = CBufferHeap()->WriteConstantBuffer(param.AlignedConstantBuffer,param.alignedCBufferSize);
                cmdList->SetGraphicsRootConstantBufferView(slotIndex,CBufferHeap()->GetAddress() + offset);
                slotIndex++;
            }

            
        }
        else
        {
            if (param.bindDesc.Type == D3D_SIT_SAMPLER)continue;
            if (param.bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D10_SIT_TEXTURE)
            {
                MaterialProperty* property = mat->FindProperty(param.name);
                if (property)
                {

                    auto colorstatus = Resource::StatusMap.find(property->texture->GetTexture());
                    if (colorstatus != Resource::StatusMap.end())
                    {
                        if (colorstatus->second.status != D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
                        {
                            D3D12_RESOURCE_BARRIER toPresent{};
                            toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            toPresent.Transition.pResource = property->texture->GetTexture();
                            toPresent.Transition.StateBefore = colorstatus->second.status;
                            toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                            toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            cmdList->ResourceBarrier(1, &toPresent);
                            colorstatus->second.status = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                        }
                    }
                     

                   cmdList->SetGraphicsRootDescriptorTable(slotIndex,property->texture->GPUHandles[int(ViewType::SRV)]);
                   slotIndex++;
                }
                else
                {
                    if (param.name.find("Normal") != std::string::npos)
                    {
                        cmdList->SetGraphicsRootDescriptorTable(slotIndex,TextureBuffer::GetDefaultNormalTex().GPUHandles[int(ViewType::SRV)]);
                    }
                    else
                    {
                        cmdList->SetGraphicsRootDescriptorTable(slotIndex,TextureBuffer::GetDefaultWhiteTex().GPUHandles[int(ViewType::SRV)]);
                    }
                   
                   slotIndex++;
                }
                
            }
        }
        
    }
    

    cmdList->IASetVertexBuffers(0, 1, mesh->VBV());
    if (mesh->indices.size() > 0)
    {
        cmdList->IASetIndexBuffer(mesh->IBV());
        cmdList->DrawIndexedInstanced(mesh->IndicesOffsets[submeshIndex].size,1,mesh->IndicesOffsets[submeshIndex].start, 0, 0);
    }
    else
    {
        cmdList->DrawInstanced(mesh->vertices.size(), 1, 0, 0);
    }
}


void Context::Dispatch(ID3D12GraphicsCommandList* cmdList,Material* mat,UINT x,UINT y,UINT z )
{
 
    ID3D12PipelineState* pso = mat->GetPso().Get();
    if (pso)
    {

    }
    else
    {
        mat->Create();
    }

    cmdList->SetComputeRootSignature(mat->GetRootSignature().Get());
    cmdList->SetPipelineState(mat->GetPso().Get());

    //设置材质属性
    auto& paramList = mat->GetComputeShader()->GetShaderResourceParams();
    UINT slotIndex = 0;
    for (size_t i = 0; i < paramList.size(); i++)
    {
        auto& param = paramList[i];
        //判断cb
        if (param.bufferDataDescs.size() > 0)
        {
           
            for (size_t j = 0; j < param.bufferDataDescs.size(); j++)
            {
                if (param.AlignedConstantBuffer == nullptr)
                param.AlignedConstantBuffer = new uint8_t[param.alignedCBufferSize]{};
                auto value = param.bufferDataDescs[j];
                auto name = param.bufferDataDescNames[j];
                MaterialProperty* property = mat->FindProperty(name);
                if (property)
                {
                    //printf("x is :  %f\n",*((float*)(property->data)));
                    memcpy(param.AlignedConstantBuffer+value.StartOffset,property->data,property->size);
                }
                else
                {
                    memset(param.AlignedConstantBuffer+value.StartOffset,0,value.Size);
                }
                
            }           
            auto offset = CBufferHeap()->WriteConstantBuffer(param.AlignedConstantBuffer,param.alignedCBufferSize);
            cmdList->SetComputeRootConstantBufferView(slotIndex,CBufferHeap()->GetAddress() + offset);
            slotIndex++;
  

            
        }
        else
        {
            if (param.bindDesc.Type == D3D_SIT_SAMPLER)continue;
            if (param.bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE)
            {
                MaterialProperty* property = mat->FindProperty(param.name);
                if (property)
                {
                   cmdList->SetComputeRootDescriptorTable(slotIndex,property->texture->GPUHandles[int(ViewType::SRV)]);
                   slotIndex++;
                }
                else
                {
                   cmdList->SetComputeRootDescriptorTable(slotIndex,TextureBuffer::GetDefaultWhiteTex().GPUHandles[int(ViewType::SRV)]);
                   slotIndex++;
                }                
            }
            else if (param.bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED || param.bindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED)
            {
                MaterialProperty* property = mat->FindProperty(param.name);
                if (property)
                {
                   
                    auto colorstatus = Resource::StatusMap.find(property->buffer->GetBuffer());
                    if (colorstatus != Resource::StatusMap.end())
                    {
                        D3D12_RESOURCE_STATES targetState = property->buffer->ReadWrite ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                        if (colorstatus->second.status != targetState)
                        {
                            D3D12_RESOURCE_BARRIER toPresent{};
                            toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            toPresent.Transition.pResource = property->buffer->GetBuffer();
                            toPresent.Transition.StateBefore = colorstatus->second.status;
                            toPresent.Transition.StateAfter = targetState;
                            toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            cmdList->ResourceBarrier(1, &toPresent);
                            colorstatus->second.status = targetState;
                        }
                    }
                    
                   cmdList->SetComputeRootDescriptorTable(slotIndex,property->buffer->GPUHandles[int(ViewType::SRV)]);
                   slotIndex++;
                }                
            }
        }
        
    }
    cmdList->Dispatch(x,y,z);
}


void Context::ShutDown()
{
    Mesh::ReleaseAllMeshs();
}

bool Context::Init(HWND hwnd, uint32_t width, uint32_t height)
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
    pContext = this;
    Resource::m_device = this;
    m_commandBufferPool = new CommandBufferPool(1);
    m_DsvHeap = new DSVDescriptorHeap(1024);
    m_RtvHeap = new RTVDescriptorHeap(1024);
    m_SrvHeap = new SRVDescriptorHeap(1024*512);
    m_CBufferHeap = new ConstantBufferHeap();
    m_CBufferHeap->init();
    m_ReadbackBufferHeap = new ReadbackBufferHeap();
    m_ReadbackBufferHeap->init();
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
    scDesc.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
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
		rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		DescriptorHeap::ViewDesc viewdesc(&rtDesc);

        m_swapChainViews[i] = m_RtvHeap->CreateView(m_backBuffers[i].Get(),viewdesc);
    }

    // 8. Fence + Win32 Event：GPU 完成后通过 Event 通知 CPU
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        return false;
    }
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) return false;

}