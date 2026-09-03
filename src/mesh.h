#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <DirectXMath.h>
#include "Resource.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;

class MyMesh : public Resource
{
private:
    ComPtr<ID3D12Resource> m_vertexBuffer;   // Default Heap，GPU 常驻
    ComPtr<ID3D12Resource> m_indexBuffer;
    ComPtr<ID3D12Resource> m_vbUpload;       // 临时 Upload Buffer
    ComPtr<ID3D12Resource> m_ibUpload;
    D3D12_VERTEX_BUFFER_VIEW m_vbv{};
    D3D12_INDEX_BUFFER_VIEW  m_ibv{};
    UINT m_vertexSize;
public:
    struct StrideAndSize
    {
        UINT stride;
        UINT size;
    };
    std::vector<XMFLOAT3> vertices;
    std::vector<UINT> indices;
    std::vector<XMFLOAT3> normal;
    std::vector<XMFLOAT3> tangent;
    std::vector<XMFLOAT3> color;
    std::vector<XMFLOAT4> uv0;
    std::vector<XMFLOAT4> uv1;
    std::vector<XMFLOAT4> uv2;
    std::vector<XMFLOAT4> uv3;
    std::vector<float> data;
    StrideAndSize VertexBufferSize();// { return m_vertexSize; }
    std::vector<D3D12_INPUT_ELEMENT_DESC> descs;
    void Upload(ID3D12GraphicsCommandList* cmdList,ID3D12Heap* heap = nullptr);
    void* GetData();
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputDesc();
    D3D12_VERTEX_BUFFER_VIEW* VBV() 
    { 
        return &m_vbv; 
    }
    D3D12_INDEX_BUFFER_VIEW* IBV() { return &m_ibv; }
    void ReleaseUploadBuffer()
    {
        m_vbUpload.Reset();
        m_ibUpload.Reset();
    }
};


