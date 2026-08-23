#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class ShaderPipeline {
public:
    bool Create(ID3D12Device* device);
    
    ID3D12RootSignature* GetRootSignature() const { return m_rootSig.Get(); }
    ID3D12PipelineState* GetPSO() const { return m_pso.Get(); }
    static constexpr int Test(const int a, const int b) { return a * b; }  // ✅

    
private:
    ComPtr<ID3D12RootSignature> m_rootSig;
    ComPtr<ID3D12PipelineState> m_pso;
};
static constexpr int aa = ShaderPipeline::Test(2, 3);  // ✅ 现在可以了