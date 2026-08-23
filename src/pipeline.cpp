#include "pipeline.h"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>  // 确保有这个

static void ShowError(const char* msg) {
    OutputDebugStringA(msg);
    MessageBoxA(nullptr, msg, "Shader Error", MB_ICONERROR);
}

// 读取文本文件
static std::string ReadShaderFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        ShowError("Failed to open shader: ");
        ShowError(filename);
        ShowError("\n");
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// 编译单个 Shader
static bool CompileShader(const std::string& source, const char* filename,
    const char* entryPoint, const char* target, ComPtr<ID3DBlob>& outBlob) {
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(source.c_str(), source.size(), filename,
        nullptr, nullptr, entryPoint, target, 0, 0, &outBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) ShowError((char*)errorBlob->GetBufferPointer());
        return false;
    }
    return true;
}

bool ShaderPipeline::Create(ID3D12Device* device) {
    // 从文件读取
    std::string shaderSource = ReadShaderFile("shaders/triangle.hlsl");
    if (shaderSource.empty()) return false;

    // 编译 VS 和 PS
    ComPtr<ID3DBlob> vsBlob, psBlob;
    if (!CompileShader(shaderSource, "triangle.hlsl", "VS", "vs_5_0", vsBlob)) return false;
    if (!CompileShader(shaderSource, "triangle.hlsl", "PS", "ps_5_0", psBlob)) return false;

    // ====== 以下 Root Signature 和 PSO 代码和阶段6完全一样 ======
    D3D12_ROOT_PARAMETER params[1] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].Descriptor.RegisterSpace = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = params;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rsBlob, rsError;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsError))) {
        if (rsError) ShowError((char*)rsError->GetBufferPointer());
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, rsBlob->GetBufferPointer(),
        rsBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)))) {
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSig.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.InputLayout = { layout, 2 };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;  // 深度小的（近）通过
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) {
        return false;
    }

    m_rootSig->SetName(L"Triangle RootSig");
    m_pso->SetName(L"Triangle PSO");
    return true;
}
