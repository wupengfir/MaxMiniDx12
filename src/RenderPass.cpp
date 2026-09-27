#include "RenderPass.h"
#include "Resource.h"
#include "Scene.h"
#include "context.h"
#include "TestResource.h"
#include "Camera.h"
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
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


            //drawscene
            Scene::CurrentScene->DrawScene(cmdList,formats);
            //

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
    formats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

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
	m_postprocessMaterial->SetValue<float>(MaterialPropertyType::FLOAT, "_Exposure", Context::GlobalSetting.Exposure);
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
//后面拿来生成cubemap的球谐
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
    m_diffuseIrradiance = new CubemapRenderTextureBuffer();
    m_diffuseIrradiance->Width = 64;
    m_diffuseIrradiance->Height = 64;
    m_diffuseIrradiance->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    m_diffuseIrradiance->CreateTexture();
    m_reflectIrradiance = new CubemapRenderTextureBuffer();
    m_reflectIrradiance->Width = 512;
    m_reflectIrradiance->Height = 512;
    m_reflectIrradiance->AllowMipmap = true;
    m_reflectIrradiance->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    m_reflectIrradiance->CreateTexture();
    m_Color = m_diffuseIrradiance;
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

	m_Color = m_diffuseIrradiance;
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
        //cmdList->ClearRenderTargetView(rtv, &clearColor.r, 0, nullptr);            
        if (m_Depth)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = ((TextureBuffer*)Resource::FindResourceAndStatus(m_Depth->GetTexture()).resource)->CPUHandles[(int)ViewType::DSV];// m_Depth->cpuhandle;   
            //cmdList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
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
                faceRotate = XMMatrixRotationY(XMConvertToRadians(90));
            break;
            case 1 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(-90));
            break;
            case 2 :
                faceRotate = XMMatrixRotationX(XMConvertToRadians(-90));
            break;
            case 3 :
                faceRotate = XMMatrixRotationX(XMConvertToRadians(90));
            break;
            case 4 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(0));
            break;
            case 5 :
                faceRotate = XMMatrixRotationY(XMConvertToRadians(180));
            break;

        }
        m_cubeMapConvoloveMaterial->SetValue(MaterialPropertyType::FLOAT4x4, "_FaceRotateMatrix", faceRotate);
        Context::pContext->DrawMesh(cmdList,TestResource::QuadMesh,0,m_cubeMapConvoloveMaterial, &(identity), Scene::CurrentScene->cameras[0],formats);


    }
    colorstatus->second.status = D3D12_RESOURCE_STATE_RENDER_TARGET;

    //反射部分
	m_Color = m_reflectIrradiance;
    colorResource = m_Color->GetTexture();
    colorstatus = Resource::StatusMap.find(colorResource);
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

    for (int mip = 0; mip < m_Color->GetTexture()->GetDesc().MipLevels; mip++)
    {
        for (int i = 0; i < 6; i++)
        {
            DXGI_RGBA clearColor{};
            int index = (int)ViewType::Count * m_Color->Depth * mip + (int)ViewType::RTV + (int)ViewType::Count * i;
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = ((TextureBuffer*)Resource::FindResourceAndStatus(colorResource).resource)->CPUHandles[index];
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);


            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            D3D12_VIEWPORT viewport{};
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = m_Color->GetTexture()->GetDesc().Width >> mip;
            viewport.Height = m_Color->GetTexture()->GetDesc().Height >> mip;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            cmdList->RSSetViewports(1, &viewport);

            D3D12_RECT scissor{};
            scissor.left = 0;
            scissor.top = 0;
            scissor.right = m_Color->GetTexture()->GetDesc().Width >> mip;
            scissor.bottom = m_Color->GetTexture()->GetDesc().Height >> mip;
            cmdList->RSSetScissorRects(1, &scissor);


            XMMATRIX identity = XMMatrixIdentity();
            XMMATRIX faceRotate;
            switch (i)
            {
            case 0:
                faceRotate = XMMatrixRotationY(XMConvertToRadians(90));
                break;
            case 1:
                faceRotate = XMMatrixRotationY(XMConvertToRadians(-90));
                break;
            case 2:
                faceRotate = XMMatrixRotationX(XMConvertToRadians(-90));
                break;
            case 3:
                faceRotate = XMMatrixRotationX(XMConvertToRadians(90));
                break;
            case 4:
                faceRotate = XMMatrixRotationY(XMConvertToRadians(0));
                break;
            case 5:
                faceRotate = XMMatrixRotationY(XMConvertToRadians(180));
                break;

            }
            m_reflectConvoloveMaterial->SetValue(MaterialPropertyType::FLOAT4x4, "_FaceRotateMatrix", faceRotate);
            m_reflectConvoloveMaterial->SetValue(MaterialPropertyType::FLOAT,"_Roughness", ((float)mip)/m_Color->GetTexture()->GetDesc().MipLevels);
            Context::pContext->DrawMesh(cmdList, TestResource::QuadMesh, 0, m_reflectConvoloveMaterial, &(identity), Scene::CurrentScene->cameras[0], formats);


        }
    }
    
    colorstatus->second.status = D3D12_RESOURCE_STATE_RENDER_TARGET;


    Material::SetGlobalTexture("_GeneratedIrradiancemap",m_diffuseIrradiance);
    Material::SetGlobalTexture("_GeneratedReflectionmap", m_reflectIrradiance);
    cmdList->Close();
    ID3D12CommandList* lists[] = {cmdList };
    queue->ExecuteCommandLists(1, lists);
}



ShadowPass::ShadowPass()
{
	m_shadowMap = std::make_unique<DepthTextureBuffer>();
	m_shadowMap->Width = 2048;
	m_shadowMap->Height = 2048;
    m_shadowMap->CreateTexture();
}

void ShadowPass::ComputeDirectionalShadowCameraMatrix(Camera* cam)
{

	float distance = XMMin(cam->GetFar(),shadowDistance)/ cam->GetFar();
	XMMATRIX& invVP = cam->InvVPMatrix();
    XMVECTOR p0 = XMVECTOR{ 0, 0, cam->GetNear(),1 };
    XMVECTOR p1 = XMVECTOR{ -1, -1, 1, 1 };
    XMVECTOR p2 = XMVECTOR{ -1, 1, 1, 1 };
    XMVECTOR p3 = XMVECTOR{ 1, 1, 1, 1 };
    XMVECTOR p4 = XMVECTOR{ 1, -1, 1, 1 };
    p0 = XMVector4Transform(p0, invVP); 
    p0 = XMVectorDivide(p0, XMVectorSplatW(p0));
    p1 = XMVector4Transform(p1, invVP); 
    p1 = XMVectorDivide(p1, XMVectorSplatW(p1)); 
    p1 = XMVectorLerp(p0, p1, distance);
    p2 = XMVector4Transform(p2, invVP); 
    p2 = XMVectorDivide(p2, XMVectorSplatW(p2)); 
    p2 = XMVectorLerp(p0, p2, distance);
    p3 = XMVector4Transform(p3, invVP); 
    p3 = XMVectorDivide(p3, XMVectorSplatW(p3)); 
    p3 = XMVectorLerp(p0, p3, distance);
    p4 = XMVector4Transform(p4, invVP); 
    p4 = XMVectorDivide(p4, XMVectorSplatW(p4)); 
    p4 = XMVectorLerp(p0, p4, distance);


    XMVECTOR sceneCenter = XMVectorZero();
    sceneCenter += p0*4;
    sceneCenter += p1;
    sceneCenter += p2;
    sceneCenter += p3;
    sceneCenter += p4;
    sceneCenter /= 8.0f;

    // 光源位置：从sceneCenter，沿着光线方向往外拉一段
    XMVECTOR lightPos = sceneCenter + XMLoadFloat4(&Context::pContext->GlobalSetting.MainLightDirection) * 1000.0f;

    // 构建 shadowView：光源看向 sceneCenter
    XMVECTOR up = XMVectorSet(0, 0, 1, 0);
    // 防止 up 和 lightDir 共线，可加一个鲁棒判断
    m_shadowView = XMMatrixLookAtLH(lightPos, sceneCenter, up);

    // ==========3. 把8个世界点变换到光源视图空间 ==========
    p0 = XMVector4Transform(p0, m_shadowView);
    p1 = XMVector4Transform(p0, m_shadowView);
    p2 = XMVector4Transform(p0, m_shadowView);
    p3 = XMVector4Transform(p0, m_shadowView);
    p4 = XMVector4Transform(p0, m_shadowView);

    XMVECTOR minp, maxp;
    minp = XMVectorMin(p0, p1); minp = XMVectorMin(minp, p2); minp = XMVectorMin(minp, p3); minp = XMVectorMin(minp, p4);
    maxp = XMVectorMax(p0, p1); minp = XMVectorMax(minp, p2); minp = XMVectorMax(minp, p3); minp = XMVectorMax(minp, p4);
    m_shadowProj = XMMatrixOrthographicOffCenterLH(
        minp.m128_f32[0], maxp.m128_f32[0],
        minp.m128_f32[1], maxp.m128_f32[1],
        minp.m128_f32[2], maxp.m128_f32[2]
    );
	m_shadowVP = m_shadowView * m_shadowProj;
}

void ShadowPass::ExecutePass(ID3D12CommandQueue* queue)
{
    CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0, Context::pContext->FrameIndex(), CommandBufferPool::Type::DIRECT);
    ID3D12GraphicsCommandList* cmdList = cmdbuffer.CmdList.Get();
    //设置描述符堆，每个cmdlist都要设
    ID3D12DescriptorHeap* ppHeaps[] = { Context::pContext->SrvHeap()->Heap() };
    cmdList->SetDescriptorHeaps(1, ppHeaps);


    Context::SetRenderTarget(cmdList, nullptr, false, { 0,0,0,0 }, m_shadowMap.get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    //drawscene
    Scene::CurrentScene->DrawSceneShadow(cmdList, &m_shadowVP);
    //

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList };
    queue->ExecuteCommandLists(1, lists);
}
