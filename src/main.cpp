#include <windows.h>
#include <windowsx.h>
#include <cwchar>
#include <algorithm>
#include <chrono>
#include "context.h"
#include "mesh.h"
#include "Resource.h"
#include "TestResource.h"
#include "Material.h"
#include "Scene.h"
#include "RenderPass.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "Camera.h"
#include "Texture.h"
#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);


static HWND          g_hwnd = nullptr;
static const uint32_t kWidth  = 1920;
static const uint32_t kHeight = 1080;
// FPS 计数
static uint64_t g_frameCount = 0;
static Camera* g_gameCamera = nullptr;
static bool g_isRotatingCamera = false;
static POINT g_lastMousePosition{};
static constexpr float kMouseSensitivity = 0.003f;
static constexpr float kCameraMoveSpeed = 2.0f;

bool IsKeyDown(int vkCode)
{
    // 最高位=1代表当前按下
    return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const bool hasImGuiContext = ImGui::GetCurrentContext() != nullptr;
    if (hasImGuiContext)
    {
        // Always forward input so ImGui can update its internal state. Whether
        // the scene should also consume it is decided by WantCaptureMouse.
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_RBUTTONDOWN:
        if (g_gameCamera &&
            (!hasImGuiContext || !ImGui::GetIO().WantCaptureMouse))
        {
            g_isRotatingCamera = true;
            g_lastMousePosition = {
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam)
            };
            SetCapture(hwnd);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_gameCamera && g_isRotatingCamera)
        {
            const POINT currentMousePosition{
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam)
            };
            const LONG deltaX = currentMousePosition.x - g_lastMousePosition.x;
            const LONG deltaY = currentMousePosition.y - g_lastMousePosition.y;
            g_lastMousePosition = currentMousePosition;

            XMFLOAT3 rotation = g_gameCamera->GetTransform().GetRotation();
            rotation.z -= static_cast<float>(deltaX) * kMouseSensitivity;
            rotation.x -= static_cast<float>(deltaY) * kMouseSensitivity;

            constexpr float maxPitch = XMConvertToRadians(89.0f);
            rotation.x = std::clamp(rotation.x, -maxPitch, maxPitch);
            g_gameCamera->SetRotation(rotation);
        }
        return 0;

    case WM_RBUTTONUP:
        if (g_isRotatingCamera)
        {
            g_isRotatingCamera = false;
            ReleaseCapture();
        }
        return 0;

    case WM_CAPTURECHANGED:
        // Capture ownership may change while ImGui is processing the same
        // mouse gesture. Do not cancel camera rotation while RMB is still down.
        if (!IsKeyDown(VK_RBUTTON))
        {
            g_isRotatingCamera = false;
        }
        return 0;

    case WM_KILLFOCUS:
        g_isRotatingCamera = false;
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        return 0;

    //case WM_SIZE:
    //    g_dx12.Resize(LOWORD(lParam), HIWORD(lParam));
    //    return 0;

    //case WM_KEYDOWN:
    //{
    //    // wParam = 虚拟键码 Virtual-Key Code
    //    UINT key = (UINT)wParam;
    //    if(key == 'A')
    //    {
    //        XMFLOAT3 pos = Scene::CurrentScene->cameras[0]->GetTransform().GetPos();
    //        pos.y -= 0.05;
    //        Scene::CurrentScene->cameras[0]->SetPos(pos);
    //    }
    //    else if (key == 'D')
    //    {
    //        XMFLOAT3 pos = Scene::CurrentScene->cameras[0]->GetTransform().GetPos();
    //        pos.y += 0.05;
    //        Scene::CurrentScene->cameras[0]->SetPos(pos);
    //    }
    //    else if (key == 'S')
    //    {
    //        XMFLOAT3 pos = Scene::CurrentScene->cameras[0]->GetTransform().GetPos();
    //        pos.x -= 0.05;
    //        Scene::CurrentScene->cameras[0]->SetPos(pos);
    //    }
    //    else if(key == 'W')
    //    {
    //       XMFLOAT3 pos = Scene::CurrentScene->cameras[0]->GetTransform().GetPos();
    //        pos.x += 0.05;
    //        Scene::CurrentScene->cameras[0]->SetPos(pos); 
    //    }
    //    break;
    //}
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void UpdateCameraMovement(Camera& camera, float deltaTime)
{
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }

    XMVECTOR localMovement = XMVectorZero();

    // The project uses X=forward, Y=right, Z=up in camera-local space.
    if (IsKeyDown('W')) localMovement = XMVectorAdd(localMovement, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
    if (IsKeyDown('S')) localMovement = XMVectorAdd(localMovement, XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f));
    if (IsKeyDown('D')) localMovement = XMVectorAdd(localMovement, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    if (IsKeyDown('A')) localMovement = XMVectorAdd(localMovement, XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));
    if (IsKeyDown('E')) localMovement = XMVectorAdd(localMovement, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
    if (IsKeyDown('Q')) localMovement = XMVectorAdd(localMovement, XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f));

    if (XMVector3LengthSq(localMovement).m128_f32[0] < 1e-6f)
    {
        return;
    }

    // Normalize so diagonal movement is not faster, then rotate the local
    // direction by the camera orientation. w=0 keeps translation out.
    localMovement = XMVector3Normalize(localMovement);
    localMovement = XMVectorScale(localMovement, kCameraMoveSpeed * deltaTime);

    std::swap(localMovement.m128_f32[0],localMovement.m128_f32[1]);

    const XMVECTOR worldMovement = XMVector4Transform(
        localMovement,
        camera.GetTransform().WorldMatrix());

    XMFLOAT3 position = camera.GetTransform().GetPos();
    position.x += worldMovement.m128_f32[0];
    position.y += worldMovement.m128_f32[1];
    position.z += worldMovement.m128_f32[2];
    camera.SetPos(position);
}

HWND CreateCustomWindow(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // 注册窗口类
    const wchar_t* className = L"MinimalDX12";
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);

    // 创建窗口（AdjustWindowRect 让客户区正好是 1280x720）
    RECT rect = {0, 0, (LONG)kWidth, (LONG)kHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowExW(0, className, L"Minimal DX12",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd, nCmdShow);
    return hwnd;
}

void AllocateConsole()
{
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow) { 
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(269);
    g_hwnd = CreateCustomWindow(hInstance, nullptr, nullptr, nCmdShow);

    AllocateConsole();

    Context currentContext;
    if (!currentContext.Init(g_hwnd, kWidth, kHeight))
    {
        MessageBoxW(g_hwnd, L"DX12 Init Failed", L"Error", MB_ICONERROR);
        return -1;
    }

    ImGuiLayer imguiLayer;
    if (!imguiLayer.Initialize(g_hwnd, currentContext))
    {
        MessageBoxW(g_hwnd, L"ImGui Init Failed", L"Error", MB_ICONERROR);
        return -1;
    }
    currentContext.CreateFrameBuffers();

    


    currentContext.BeginFrame();
    //创建一些默认资源
    InitEngineResource();

    CommandBuffer cmdbuffer = currentContext.GetCommandBufferPool()->AcquireCommandList(0,currentContext.FrameIndex(),CommandBufferPool::Type::DIRECT);
    
    
    Mesh* mesh01;
    Mesh* mesh02;
    Mesh* helmetMesh = Mesh::Load("resource/mesh/sci_fi_space_helmet_by_aliashasim.FBX");
    TestResource::QuadMesh = TestResource::GetMeshQuad();
    mesh01 = Mesh::Load("resource/mesh/ball.FBX");
    mesh02 = Mesh::Load("resource/mesh/tree01.FBX");
    mesh01->Upload(cmdbuffer.CmdList.Get());
    mesh02->Upload(cmdbuffer.CmdList.Get());
    helmetMesh->Upload(cmdbuffer.CmdList.Get());
    TestResource::QuadMesh->Upload(cmdbuffer.CmdList.Get());



    Texture tex01;
    tex01.LoadTexture(L"resource/texture/1.tga",true,true);
    tex01.UploadTexture(tex01.GetImage(), cmdbuffer.CmdList.Get());

    Texture texCube;
    texCube.LoadTexture(L"resource/texture/sky_cube.dds",true,false);
    texCube.UploadTexture(texCube.GetImage(), cmdbuffer.CmdList.Get());

    Texture albedoTex;
    albedoTex.LoadTexture(L"resource/texture/1k/SFSHelmet_bcolor.png",true,true);
    albedoTex.UploadTexture(albedoTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture metallicTex;
    metallicTex.LoadTexture(L"resource/texture/1k/SFSHelmet_metal.png",true,false);
    metallicTex.UploadTexture(metallicTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture roughnessTex;
    roughnessTex.LoadTexture(L"resource/texture/1k/SFSHelmet_rough.png",true,false);
    roughnessTex.UploadTexture(roughnessTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture normalTex;
    normalTex.LoadTexture(L"resource/texture/1k/SFSHelmet_norm.png",true,false);
    normalTex.UploadTexture(normalTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture iblIrradianceTex;
    iblIrradianceTex.LoadTexture(L"resource/texture/ibl_irradiance_cube.dds", true, false);
    iblIrradianceTex.UploadTexture(iblIrradianceTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture iblSpecularIrradianceTex;
    iblSpecularIrradianceTex.LoadTexture(L"resource/texture/ibl_specular_cube.dds", true, false);
    iblSpecularIrradianceTex.UploadTexture(iblSpecularIrradianceTex.GetImage(), cmdbuffer.CmdList.Get());

    Texture iblBrdfTex;
    iblBrdfTex.LoadTexture(L"resource/texture/brdf_look_up_table.dds", true, false);
    iblBrdfTex.UploadTexture(iblBrdfTex.GetImage(), cmdbuffer.CmdList.Get());

    /*StructureBuffer uavBuffer;
    uavBuffer.ReadWrite = true;
    uavBuffer.Width = 256;
    uavBuffer.Stride = sizeof(XMFLOAT3);
    uavBuffer.CreateBuffer();*/

    //Compute Shader
    ComputeShader sampleCS("shaders/SampleCs.compute");
    sampleCS.LoadShader();
    //Shader   
    Shader shader("shaders/skybox.hlsl");
    shader.LoadShader();
    Shader litShader("shaders/Lit.Shader");
    litShader.LoadShader();
    Shader postShader("shaders/postprocess.hlsl");
    postShader.LoadShader();
    Shader cubeMapConvoloveShader("shaders/CubeMapConvolove.shader");
    cubeMapConvoloveShader.LoadShader();
    Shader cubeMapReflectionConvoloveShader("shaders/CubeMapReflectionConvolove.shader");
    cubeMapReflectionConvoloveShader.LoadShader();
    //创建材质

    Material mat;
    Material mat1;
    Material mat2;
    Material pbrMat;
    Material postprocessMat;
    Material computeMat;
    Material cubeMapConvoloveMaterial;
    Material cubeMapReflectionConvoloveMaterial;

    mat.SetShader(&shader);
    mat1.SetShader(&litShader);
    mat2.SetShader(&litShader);
    pbrMat.SetShader(&litShader);
    postprocessMat.SetShader(&postShader);
    computeMat.SetComputeShader(&sampleCS);
    cubeMapConvoloveMaterial.SetShader(&cubeMapConvoloveShader);
    cubeMapReflectionConvoloveMaterial.SetShader(&cubeMapReflectionConvoloveShader);

    postprocessMat.DepthEnable = false;
    mat2.SetValue<XMFLOAT4>(MaterialPropertyType::FLOAT4,"myFloat4", XMFLOAT4{0.5,0.2,0,0});
    mat.SetValue<XMFLOAT4>(MaterialPropertyType::FLOAT4,"tempData", XMFLOAT4{0.2,0.7,0.2,0});
    mat.SetTexture("tex01",&tex01);
    mat.SetTexture("cubemap",&texCube);
    mat1.SetTexture("tex01",&tex01);
    mat1.SetValue<XMFLOAT4>(MaterialPropertyType::FLOAT4,"testcolor", XMFLOAT4{0.9,0.2,0.2,0});
    pbrMat.SetTexture("_BaseMap",&albedoTex);
    pbrMat.SetTexture("_MetallicMap",&metallicTex);
    pbrMat.SetTexture("_RoughnessMap",&roughnessTex);
    pbrMat.SetTexture("_NormalMap",&normalTex);
    pbrMat.SetTexture("_IrradianceMap", &iblIrradianceTex);
    pbrMat.SetTexture("_ReflectionMap", &iblSpecularIrradianceTex);
    pbrMat.SetTexture("_BrdfMap", &iblBrdfTex);
    cubeMapConvoloveMaterial.SetTexture("cubemap",&texCube);
    cubeMapConvoloveMaterial.CullMode = D3D12_CULL_MODE_NONE;
    cubeMapConvoloveMaterial.DepthEnable = false;
    cubeMapConvoloveMaterial.BlendEnable = true;

    cubeMapReflectionConvoloveMaterial.SetTexture("cubemap", &texCube);
    cubeMapReflectionConvoloveMaterial.CullMode = D3D12_CULL_MODE_NONE;
    cubeMapReflectionConvoloveMaterial.DepthEnable = false;
    cubeMapReflectionConvoloveMaterial.BlendEnable = true;


    mat1.CullMode = D3D12_CULL_MODE_NONE;

    postprocessMat.DepthEnable = false;
    postprocessMat.CullMode = D3D12_CULL_MODE_NONE;
    postprocessMat.DepthTest = D3D12_COMPARISON_FUNC_ALWAYS;

    cmdbuffer.CmdList->Close();
    ID3D12CommandList* lists[] = { cmdbuffer.CmdList.Get()};
    currentContext.QueueDirect()->ExecuteCommandLists(1, lists);

    currentContext.EndFrame();
    currentContext.WaitForGpu();
    //mesh01->ReleaseUploadBuffer();
    //mesh02->ReleaseUploadBuffer();
    //texture->ReleaseUploadBuffer();




    std::unique_ptr<Camera> gameCamera = std::make_unique<Camera>();
    g_gameCamera = gameCamera.get();
    Scene scene;
    scene.cameras.push_back(gameCamera.get());
    gameCamera->SetPos(XMFLOAT3(-2,0,0));

    GameObject obj0;
    obj0.mesh = mesh01;
    obj0.materials.push_back(& mat);
    obj0.transform.SetScale({1000,1000,1000});

    GameObject obj1;
    obj1.mesh = mesh02;
    obj1.materials.push_back(& mat1);
    obj1.materials.push_back(& mat2);
    obj1.transform.SetScale({0.01,0.01,0.01});
    obj1.transform.SetPos({ 15, 0, -10 });
    obj1.transform.SetRotation({XMConvertToRadians(90),0,0});
    

    GameObject obj2;
    obj2.mesh = mesh01;
    obj2.materials.push_back(& pbrMat);
    obj2.transform.SetScale({0.01,0.01,0.01});
    obj2.transform.SetPos({ 2, 0, 10 });

    GameObject helmetObj;
    helmetObj.mesh = helmetMesh;
    helmetObj.materials.push_back(& pbrMat);
    helmetObj.transform.SetScale({0.1,0.1,0.1});
    helmetObj.transform.SetPos({ 0, 3, -15 });
    helmetObj.transform.SetRotation({XMConvertToRadians(90),0,0});


    OpaquePass opaquePass;
    PostProcessPass postPass;
    EnvironmetConvolovePass convolovePass;
    ReadBackPass readBakcPass;
    CubemapConvolovePass cubemapConvolovePass;
    postPass.SetPostMaterial(&postprocessMat);
    convolovePass.SetConvoloveMaterial(&computeMat);
    cubemapConvolovePass.SetConvoloveMaterial(&cubeMapConvoloveMaterial);
    cubemapConvolovePass.SetReflectConvoloveMaterial(&cubeMapReflectionConvoloveMaterial);

    MSG msg{};
    auto previousFrameTime = std::chrono::steady_clock::now();
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {

            const auto currentFrameTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(
                currentFrameTime - previousFrameTime).count();
            previousFrameTime = currentFrameTime;
            // Avoid a large jump after a breakpoint, window drag, or pause.
            deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);
            if (g_gameCamera)
            {
                UpdateCameraMovement(*g_gameCamera, deltaTime);
            }

            //每帧更新逻辑,后面移走
            currentContext.Update();
            /*XMFLOAT3 rotation = obj0.transform.GetRotation();
            rotation.z += 0.01;
            obj0.transform.SetRotation(rotation);*/
            ////////////////////////////////////////////////////


            currentContext.BeginFrame();
            imguiLayer.BeginFrame();
            imguiLayer.DrawExampleWindow(deltaTime);

            //DescPtr* rtDesc = &logicalDevice.RtvHeap()->DescResourceMap()[logicalDevice.GetBackBuffer(logicalDevice.FrameIndex())];
            //DescPtr* depthDesc = &logicalDevice.DsvHeap()->DescResourceMap()[depthbuffer.GetTexture()];
            //DescPtr* colorDesc = &logicalDevice.RtvHeap()->DescResourceMap()[colorbuffer.GetTexture()];
            readBakcPass.ExecutePass(currentContext.QueueDirect());
            opaquePass.SetRenderTaget(&currentContext.colorbuffer,&currentContext.depthbuffer);
            opaquePass.ExecutePass(currentContext.QueueDirect());
            postPass.SetRenderTaget(nullptr,&currentContext.depthbuffer);
            postPass.ExecutePass(currentContext.QueueDirect());
            convolovePass.ExecutePass(currentContext.QueueDirect());
            cubemapConvolovePass.ExecutePass(currentContext.QueueDirect());
            imguiLayer.Render(currentContext);
            currentContext.EndFrame();
            //// 阶段3：FPS 计数，每秒更新一次标题栏
            //g_frameCount++;
            //uint64_t now = GetTickCount64();
            //if (now - g_lastFpsTime >= 1000) {
            //    uint32_t fps = (uint32_t)(g_frameCount * 1000 / (now - g_lastFpsTime));
            //    wchar_t title[256];
            //    swprintf_s(title, L"Minimal DX12 - FPS: %u  (%ux%u)",
            //        fps, currentContext.ScreenWidth, currentContext.ScreenHeight);
            //    SetWindowText(g_hwnd, title);
            //    g_frameCount = 0;
            //    g_lastFpsTime = now;
            //}
        }
    }

    g_gameCamera = nullptr;
    currentContext.WaitForGpu();
    imguiLayer.Shutdown();
    currentContext.ShutDown();
    //_CrtDumpMemoryLeaks();
    return 0;


}
