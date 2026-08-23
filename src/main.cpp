#include <windows.h>
#include <cwchar>
#include "dx12_context.h"

#include "mesh.h"
#include "pipeline.h"

static ShaderPipeline g_pipeline;
static Mesh g_triangleFront;  // 前面的三角形（红色）
static Mesh g_triangleBack;   // 后面的三角形（蓝色）

// 前面：z=0.2（更靠近相机），红色，偏左
static const Vertex kFrontVertices[] = {
    {{-0.3f,  0.5f, 0.2f}, {1.0f, 0.0f, 0.0f}},
    {{-0.8f, -0.5f, 0.2f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.2f, -0.5f, 0.2f}, {1.0f, 0.0f, 0.0f}},
};
static const uint32_t kFrontIndices[] = { 0, 2, 1 };

// 后面：z=0.8（更远），蓝色，偏右
static const Vertex kBackVertices[] = {
    {{ 0.3f,  0.5f, 0.8f}, {0.0f, 0.0f, 1.0f}},
    {{-0.2f, -0.5f, 0.8f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.8f, -0.5f, 0.8f}, {0.0f, 0.0f, 1.0f}},
};
static const uint32_t kBackIndices[] = { 0, 2, 1 };

static DX12Context   g_dx12;
static HWND          g_hwnd = nullptr;
static const uint32_t kWidth  = 1920;
static const uint32_t kHeight = 1080;
// FPS 计数
static uint64_t g_frameCount = 0;
static uint64_t g_lastFpsTime = 0;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        g_dx12.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow) {
    //不随windows的ui缩放变化
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
    g_hwnd = hwnd;  // 新增：保存全局句柄
    // 阶段3：先开 Debug Layer，再 Init
    DX12Context::EnableDebugLayer();
    // 初始化 DX12
    if (!g_dx12.Init(hwnd, kWidth, kHeight)) {
        MessageBoxW(hwnd, L"DX12 Init Failed", L"Error", MB_ICONERROR);
        return -1;
    }

    // 阶段6：创建 Root Signature + PSO
    if (!g_pipeline.Create(g_dx12.GetDevice())) {
        MessageBoxW(hwnd, L"Pipeline Create Failed", L"Error", MB_ICONERROR);
        return -1;
    }
    g_dx12.CreateDepthBuffer();  // 新增
    // 阶段5：上传顶点
    {
        auto* cmdList = g_dx12.BeginFrame();
        g_triangleFront.Upload(g_dx12.GetDevice(), cmdList, kFrontVertices, 3, kFrontIndices, 3);
        g_triangleBack.Upload(g_dx12.GetDevice(), cmdList, kBackVertices, 3, kBackIndices, 3);
        g_dx12.EndFrame();
        g_dx12.WaitForGpu();
        g_triangleFront.ReleaseTemporary();
        g_triangleBack.ReleaseTemporary();
    }

    g_lastFpsTime = GetTickCount64();  // 新增
    // 消息循环（阶段2才会在 else 分支里加渲染）
    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            auto* cmdList = g_dx12.BeginFrame();

            // === 录制命令 ===

            // 1. BackBuffer 从 PRESENT 状态过渡到 RENDER_TARGET（才能写）
            D3D12_RESOURCE_BARRIER toRender{};
            toRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            toRender.Transition.pResource = g_dx12.GetBackBuffer(g_dx12.GetFrameIndex());
            toRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            toRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            toRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &toRender);

            // 清屏
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_dx12.GetCurrentBackBufferRTV();
            float clearColor[] = { 0.2f, 0.4f, 0.6f, 1.0f };
            cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

            // 新增：清深度（每帧重置为最远 1.0）
            cmdList->ClearDepthStencilView(g_dx12.GetDSV(),
                D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            // 绑定 RT + DSV（第四个参数传 DSV 句柄）
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_dx12.GetDSV();
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

            // 设置管线状态
            cmdList->SetGraphicsRootSignature(g_pipeline.GetRootSignature());
            cmdList->SetPipelineState(g_pipeline.GetPSO());

            D3D12_VIEWPORT viewport{};
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = (float)g_dx12.GetWidth();
            viewport.Height = (float)g_dx12.GetHeight();
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            cmdList->RSSetViewports(1, &viewport);

            D3D12_RECT scissor{};
            scissor.left = 0;
            scissor.top = 0;
            scissor.right = g_dx12.GetWidth();
            scissor.bottom = g_dx12.GetHeight();
            cmdList->RSSetScissorRects(1, &scissor);

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 画两个三角形（顺序无所谓，深度测试会处理遮挡）
            g_triangleFront.Draw(cmdList);
            g_triangleBack.Draw(cmdList);
            

            // 3. 过渡回 PRESENT 状态（才能 Present 到屏幕）
            D3D12_RESOURCE_BARRIER toPresent{};
            toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            toPresent.Transition.pResource = g_dx12.GetBackBuffer(g_dx12.GetFrameIndex());
            toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &toPresent);

            g_dx12.EndFrame();
            // 阶段3：FPS 计数，每秒更新一次标题栏
            g_frameCount++;
            uint64_t now = GetTickCount64();
            if (now - g_lastFpsTime >= 1000) {
                uint32_t fps = (uint32_t)(g_frameCount * 1000 / (now - g_lastFpsTime));
                wchar_t title[256];
                swprintf_s(title, L"Minimal DX12 - FPS: %u  (%ux%u)",
                    fps, g_dx12.GetWidth(), g_dx12.GetHeight());
                SetWindowText(g_hwnd, title);
                g_frameCount = 0;
                g_lastFpsTime = now;
            }
        }
    }

    g_dx12.Shutdown();
    return 0;
}
