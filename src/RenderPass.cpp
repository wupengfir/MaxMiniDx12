#include "RenderPass.h"
#include "Resource.h"
#include "Scene.h"
#include "context.h"
#include "TestResource.h"
#include <iostream>
void RenderPass::SetRenderTaget(TextureBuffer* rt, TextureBuffer* depth)
{
	m_Color = rt;
	m_Depth = depth;
    //if(depth)
    //    m_DepthDesc = depth->GetTexture()->GetDesc();
    //m_ColorDescs.clear();
    //m_ColorDescs.push_back(rt->GetTexture()->GetDesc());
}

void OpaquePass::ExecutePass(ID3D12CommandQueue* queue)
{
            CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0,Context::pContext->FrameIndex(),CommandBufferPool::Type::DIRECT);
            ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
            //设置描述符堆，每个cmdlist都要设
            ID3D12DescriptorHeap* ppHeaps[] = { Context::pContext->SrvHeap()->Heap() };
	        cmdList->SetDescriptorHeaps(1, ppHeaps);

            DXGI_FORMAT formats[8] = {};
            formats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

            ColorAttachment = m_Color->GetTexture();
            Context::SetRenderTarget(cmdList, m_Color, true, { 0,0,0,0 }, m_Depth);
            //auto colorstatus = Resource::StatusMap.find(m_Color);
            //if (colorstatus == Resource::StatusMap.end())
            //{
            //    return;
            //}
            //if (colorstatus->second.status != D3D12_RESOURCE_STATE_RENDER_TARGET)
            //{
            //    // 1. BackBuffer 从 PRESENT 状态过渡到 RENDER_TARGET（才能写）
            //    D3D12_RESOURCE_BARRIER toRender{};
            //    toRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            //    toRender.Transition.pResource = m_Color;
            //    //toRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            //    toRender.Transition.StateBefore = colorstatus->second.status;
            //    toRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            //    toRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            //    cmdList->ResourceBarrier(1, &toRender);
            //    colorstatus->second.status = D3D12_RESOURCE_STATE_RENDER_TARGET;
            //}
	        

            //// 清屏
            //D3D12_CPU_DESCRIPTOR_HANDLE rtv = ((TextureBuffer*)Resource::FindResourceAndStatus(m_Color).resource)->CPUHandles[(int)ViewType::RTV];// m_Color->cpuhandle;
            //D3D12_CPU_DESCRIPTOR_HANDLE dsv = ((TextureBuffer*)Resource::FindResourceAndStatus(m_Depth).resource)->CPUHandles[(int)ViewType::DSV];// m_Depth->cpuhandle;
            //float clearColor[] = { 0,0,0,0 };
            //cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);            
            //cmdList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            //cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

            //D3D12_VIEWPORT viewport{};
            //viewport.TopLeftX = 0;
            //viewport.TopLeftY = 0;
            //viewport.Width = m_ColorDescs[0].Width;
            //viewport.Height = m_ColorDescs[0].Height;
            //viewport.MinDepth = 0.0f;
            //viewport.MaxDepth = 1.0f;
            //cmdList->RSSetViewports(1, &viewport);

            //D3D12_RECT scissor{};
            //scissor.left = 0;
            //scissor.top = 0;
            //scissor.right = m_ColorDescs[0].Width;
            //scissor.bottom = m_ColorDescs[0].Height;
            //cmdList->RSSetScissorRects(1, &scissor);


            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


            //drawscene
            Scene::CurrentScene->DrawScene(cmdList,cameraIndex,formats);
            //


            // // 3. 过渡回 PRESENT 状态（才能 Present 到屏幕）
            //D3D12_RESOURCE_BARRIER toPresent{};
            //toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            //toPresent.Transition.pResource = m_Color->resource;
            //toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            //toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            //toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            //cmdList->ResourceBarrier(1, &toPresent);


            cmdList->Close();
            ID3D12CommandList* lists[] = {cmdList };
            queue->ExecuteCommandLists(1, lists);

}

void PostProcessPass::ExecutePass(ID3D12CommandQueue* queue)
{
    if (!m_postprocessMaterial)
        return;
    CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0,Context::pContext->FrameIndex(),CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
    //设置描述符堆，每个cmdlist都要设
    ID3D12DescriptorHeap* ppHeaps[] = { Context::pContext->SrvHeap()->Heap() };
	cmdList->SetDescriptorHeaps(1, ppHeaps);

    DXGI_FORMAT formats[8] = {};
    formats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    ID3D12Resource* colorAttachment = Context::pContext->GetBackBuffer(Context::pContext->FrameIndex());
    // 1. BackBuffer 从 PRESENT 状态过渡到 RENDER_TARGET（才能写）
    D3D12_RESOURCE_BARRIER toRender{};
    toRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRender.Transition.pResource = colorAttachment;
    toRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &toRender);


     // 清屏
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = Context::pContext->GetSwapChainView().cpuhandle;
    //D3D12_CPU_DESCRIPTOR_HANDLE dsv = ((TextureBuffer*)Resource::FindResourceAndStatus(m_Depth).resource)->CPUHandles[(int)ViewType::DSV];// m_Depth->cpuhandle;
    float clearColor[] = { 0,0.2,0.4,0 };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);            
    //cmdList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtv, FALSE,  nullptr);

    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = colorAttachment->GetDesc().Width;
    viewport.Height = colorAttachment->GetDesc().Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor{};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = colorAttachment->GetDesc().Width;
    scissor.bottom = colorAttachment->GetDesc().Height;
    cmdList->RSSetScissorRects(1, &scissor);


    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    //drawscene
    auto colorstatus = Resource::StatusMap.find(OpaquePass::ColorAttachment);
    if (colorstatus == Resource::StatusMap.end())
    {
        return;
    }
    m_postprocessMaterial->SetTexture("_ColorAttachment",reinterpret_cast<TextureBuffer*>(colorstatus->second.resource),cmdList); 
    XMMATRIX identity = XMMatrixIdentity();
    Context::pContext->DrawMesh(cmdList,TestResource::QuadMesh,0,m_postprocessMaterial, &(identity), Scene::CurrentScene->cameras[0],formats);
    //


        // 3. 过渡回 PRESENT 状态（才能 Present 到屏幕）
    D3D12_RESOURCE_BARRIER toPresent{};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = colorAttachment;
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &toPresent);


    cmdList->Close();
    ID3D12CommandList* lists[] = {cmdList };
    queue->ExecuteCommandLists(1, lists);

}

void EnvironmetConvolovePass::ExecutePass(ID3D12CommandQueue* queue)
{
    if (!m_convoloveMaterial)
        return;
    
    if (!uavBuffer)
    {
        uavBuffer = new StructureBuffer();
        uavBuffer->ReadWrite = true;
        uavBuffer->Width = 256;
        uavBuffer->Stride = sizeof(XMFLOAT3);
        uavBuffer->CreateBuffer();

        ReadBackFunction f;
        f.resource = uavBuffer->GetBuffer();
        f.type = ReadBackType::UAVBuffer;
        f.size = uavBuffer->Width * uavBuffer->Stride;
        f.callback = [](UINT64 offset)
        {
            XMFLOAT3* data = reinterpret_cast<XMFLOAT3*>(Context::pContext->ReadBufferHeap()->GetPointer(offset));
            std::cout << "(" << (data+16)->x << ", " << (data+16)->y << ", " << (data+16)->z << ")" << std::endl;
        };
        ReadBackPass::syncCallbacks.push_back(f);
        
        m_convoloveMaterial->SetValue(MaterialPropertyType::FLOAT ,"g_DeltaTime", 0.25f);
        m_convoloveMaterial->SetBuffer("g_OutputBuffer",uavBuffer);
    }
     

    CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0,Context::pContext->FrameIndex(),CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
    //设置描述符堆，每个cmdlist都要设
    ID3D12DescriptorHeap* ppHeaps[] = { Context::pContext->SrvHeap()->Heap() };
	cmdList->SetDescriptorHeaps(1, ppHeaps);
    Context::pContext->Dispatch(cmdList, m_convoloveMaterial, 4, 1, 1);



    cmdList->Close();
    ID3D12CommandList* lists[] = {cmdList };
    queue->ExecuteCommandLists(1, lists);
}






void ReadBackPass::ExecutePass(ID3D12CommandQueue* queue)
{
    CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0,Context::pContext->FrameIndex(),CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
    for (ReadBackFunction f : syncCallbacks)
    {
        UINT64 offset = Context::pContext->ReadBufferHeap()->CopyResourceSync(&f,cmdList);
        f.callback(offset);
    }
    if(syncCallbacks.size() == 0)
        cmdList->Close();
    syncCallbacks.clear();
}

CubemapConvolovePass::CubemapConvolovePass()
{
    m_cubeMap = new CubemapRenderTextureBuffer();
    m_cubeMap->Width = 256;
    m_cubeMap->Height = 256;
    m_cubeMap->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    m_cubeMap->CreateTexture();
    m_Color = m_cubeMap;
    m_Depth = nullptr;
}

void CubemapConvolovePass::ExecutePass(ID3D12CommandQueue* queue)
{
    CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0,Context::pContext->FrameIndex(),CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
    //设置描述符堆，每个cmdlist都要设
    ID3D12DescriptorHeap* ppHeaps[] = { Context::pContext->SrvHeap()->Heap() };
	cmdList->SetDescriptorHeaps(1, ppHeaps);

    DXGI_FORMAT formats[8] = {};
    formats[0] = m_Color->Format;


    ID3D12Resource* colorResource = m_Color->GetTexture();
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
    }


    for (int i = 0; i < 6; i++)
    {

            // 清屏
        DXGI_RGBA clearColor{};
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = ((TextureBuffer*)Resource::FindResourceAndStatus(colorResource).resource)->CPUHandles[(int)ViewType::RTV + (int)ViewType::Count*i ];
        cmdList->ClearRenderTargetView(rtv, &clearColor.r, 0, nullptr);            
        if (m_Depth)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = ((TextureBuffer*)Resource::FindResourceAndStatus(m_Depth->GetTexture()).resource)->CPUHandles[(int)ViewType::DSV];// m_Depth->cpuhandle;   
            cmdList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        }
        else
        {
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        }

         cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = m_Color->GetTexture()->GetDesc().Width;
        viewport.Height = m_Color->GetTexture()->GetDesc().Height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor{};
        scissor.left = 0;
        scissor.top = 0;
        scissor.right = m_Color->GetTexture()->GetDesc().Width;
        scissor.bottom = m_Color->GetTexture()->GetDesc().Height;
        cmdList->RSSetScissorRects(1, &scissor);


        XMMATRIX identity = XMMatrixIdentity();
        XMMATRIX faceRotate;
        switch (i)
        {
            case 0 :
                faceRotate = identity;
            break;
            case 1 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(180));
            break;
            case 2 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(-90));
            break;
            case 3 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(90));
            break;
            case 4 :
                faceRotate = XMMatrixRotationZ(XMConvertToRadians(90));
            break;
            case 5 :
                faceRotate = XMMatrixRotationZ(XMConvertToRadians(-90));
            break;

        }
        m_cubeMapConvoloveMaterial->SetValue(MaterialPropertyType::FLOAT4x4, "_FaceRotateMatrix", faceRotate);
        Context::pContext->DrawMesh(cmdList,TestResource::QuadMesh,0,m_cubeMapConvoloveMaterial, &(identity), Scene::CurrentScene->cameras[0],formats);


    }

    colorstatus->second.status = D3D12_RESOURCE_STATE_RENDER_TARGET;
    Material::SetGlobalTexture("generateCubemap",m_Color);
    cmdList->Close();
    ID3D12CommandList* lists[] = {cmdList };
    queue->ExecuteCommandLists(1, lists);
}
