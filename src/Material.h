#pragma once
#include "Resource.h"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
static void ShowError(const char* msg) {
    OutputDebugStringA(msg);
    MessageBoxA(nullptr, msg, "Shader Error", MB_ICONERROR);
}

class Shader
{
private:
	std::string m_path;
    std::string m_content;
    ComPtr<ID3DBlob> m_vsBlob;
    ComPtr<ID3DBlob> m_psBlob;
    std::vector<D3D12_SHADER_INPUT_BIND_DESC> m_vsInputDesc;
    std::vector<D3D12_SHADER_INPUT_BIND_DESC> m_psInputDesc;
public:
	Shader(const std::string&& path) :m_path(path) {}

    std::vector<D3D12_SHADER_INPUT_BIND_DESC>& GetVsInputDesc() { return m_vsInputDesc; };
    std::vector<D3D12_SHADER_INPUT_BIND_DESC>& GetPsInputDesc() { return m_psInputDesc; };
    const std::string& GetPath() { return m_path; };
    const std::string& GetContent() { return m_content; };
    ComPtr<ID3DBlob>& GetVsBlob() { return m_vsBlob; }
    ComPtr<ID3DBlob>& GetPsBlob() { return m_psBlob; }
    bool LoadShader()
    {
        std::ifstream file(m_path);
        if (!file.is_open()) {
            ShowError("Failed to open shader: ");
            ShowError(m_path.c_str());
            ShowError("\n");
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        m_content = std::move(ss.str());
        return true;
    }

	bool Compile(const std::string& source, const char* filename,
    const char* entryPoint, const char* target,ComPtr<ID3DBlob>& blob) {
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(source.c_str(), source.size(), filename,
        nullptr, nullptr, entryPoint, target, 0, 0, &blob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) ShowError((char*)errorBlob->GetBufferPointer());
        return false;
    }
    return true;
}
};

//class PSOManager
//{
//public:
//    inline static std::unordered_map<Material*, ID3D12PipelineState*> PsoMap = {};
//};

class Material : public Resource
{
protected:
	ComPtr<ID3D12RootSignature> m_rootSig;
    ComPtr<ID3D12PipelineState> m_pso;
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
    Shader* m_shader;
    std::unordered_map<std::string, void*> m_MaterialProperties;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc{};
public:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& PsoDesc() { return m_psoDesc; }
	ComPtr<ID3D12RootSignature>& GetRootSignature() { return m_rootSig; }
	ComPtr<ID3D12PipelineState>& GetPso() { return m_pso; }
    void SetShader(Shader* shader) { m_shader = shader; }
    bool Create();
};