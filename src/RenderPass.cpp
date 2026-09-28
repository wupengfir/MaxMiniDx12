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
    Material::SetGlobalTexture("_ShadowMap", m_shadowMap.get());
}

void ShadowPass::ComputeDirectionalShadowCameraMatrix(Camera* cam)
{
    const float cameraNear = cam->GetNear();
    const float cameraFar = cam->GetFar();
    const float shadowFar = shadowDistance < cameraFar ? shadowDistance : cameraFar;
    const float cameraDepth = cameraFar - cameraNear;
    const float sliceRatio = cameraDepth > 0.0001f
        ? XMMin(XMMax((shadowFar - cameraNear) / cameraDepth, 0.0f), 1.0f)
        : 1.0f;

    const XMMATRIX& invVP = cam->InvVPMatrix();
    const float cornerX[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
    const float cornerY[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
    XMVECTOR frustumCorners[8];

    for (int i = 0; i < 4; ++i)
    {
        XMVECTOR nearCorner = XMVector4Transform(
            XMVectorSet(cornerX[i], cornerY[i], 0.0f, 1.0f), invVP);
        nearCorner = XMVectorDivide(nearCorner, XMVectorSplatW(nearCorner));

        XMVECTOR farCorner = XMVector4Transform(
            XMVectorSet(cornerX[i], cornerY[i], 1.0f, 1.0f), invVP);
        farCorner = XMVectorDivide(farCorner, XMVectorSplatW(farCorner));

        frustumCorners[i] = nearCorner;
        frustumCorners[i + 4] = XMVectorLerp(nearCorner, farCorner, sliceRatio);
    }

    XMVECTOR sceneCenter = XMVectorZero();
    for (const XMVECTOR& corner : frustumCorners)
    {
        sceneCenter = XMVectorAdd(sceneCenter, corner);
    }
    sceneCenter = XMVectorScale(sceneCenter, 1.0f / 8.0f);

    XMVECTOR lightDir = XMLoadFloat4(&Context::pContext->GlobalSetting.MainLightDirection);
    lightDir = XMVectorSetW(lightDir, 0.0f);
    if (XMVectorGetX(XMVector3LengthSq(lightDir)) < 0.000001f)
    {
        lightDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    lightDir = XMVector3Normalize(lightDir);

    float frustumRadius = 0.0f;
    for (const XMVECTOR& corner : frustumCorners)
    {
        frustumRadius = XMMax(
            frustumRadius,
            XMVectorGetX(XMVector3Length(XMVectorSubtract(corner, sceneCenter))));
    }

    const float depthPadding = XMMax(10.0f, shadowFar * 0.1f);
    const XMVECTOR lightPos = XMVectorAdd(
        sceneCenter, XMVectorScale(lightDir, frustumRadius + depthPadding));

    const XMVECTOR worldUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    const float upAlignment = XMVectorGetX(XMVector3Dot(lightDir, worldUp));
    const XMVECTOR up = fabsf(upAlignment) > 0.99f
        ? XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        : worldUp;
    m_shadowView = XMMatrixLookAtLH(lightPos, sceneCenter, up);

    XMVECTOR minPoint = XMVector4Transform(frustumCorners[0], m_shadowView);
    XMVECTOR maxPoint = minPoint;
    for (int i = 1; i < 8; ++i)
    {
        const XMVECTOR lightSpaceCorner = XMVector4Transform(frustumCorners[i], m_shadowView);
        minPoint = XMVectorMin(minPoint, lightSpaceCorner);
        maxPoint = XMVectorMax(maxPoint, lightSpaceCorner);
    }

    const float nearZ = XMMax(0.01f, XMVectorGetZ(minPoint) - depthPadding);
    const float farZ = XMVectorGetZ(maxPoint) + depthPadding;
    m_shadowProj = XMMatrixOrthographicOffCenterLH(
        XMVectorGetX(minPoint), XMVectorGetX(maxPoint),
        XMVectorGetY(minPoint), XMVectorGetY(maxPoint),
        nearZ, farZ);
    m_shadowVP = m_shadowView * m_shadowProj;
    Material::SetGlobalValue(MaterialPropertyType::FLOAT4x4, "_ShadowMatrix_VP", m_shadowVP);
    m_shadowMaterial->SetValue(MaterialPropertyType::FLOAT4, "_ShadowBias", Context::GlobalSetting.ShadowBias);
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
    Scene::CurrentScene->DrawSceneShadow(cmdList, &m_shadowVP,m_shadowMaterial);
    //
    SetResourceBarrier(m_shadowMap->GetTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, cmdList);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList };
    queue->ExecuteCommandLists(1, lists);
}
