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
// 顶点格式：位置 + 颜色
struct Vertex {
    float pos[3];
    float color[3];
};

class Mesh {
public:
    // 把顶点/索引数据上传到 GPU Default Heap
    // cmdList: 用于录制拷贝命令，调用者需在 Upload 后提交并 WaitForGpu
    void Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
        const Vertex* vertices, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount);

    // WaitForGpu 之后调用，释放临时的 Upload Buffer
    void ReleaseTemporary();

    // 绑定顶点/索引缓冲并 Draw（阶段7有了 PSO 后才用）
    void Draw(ID3D12GraphicsCommandList* cmdList) const;

private:
    ComPtr<ID3D12Resource> m_vertexBuffer;   // Default Heap，GPU 常驻
    ComPtr<ID3D12Resource> m_indexBuffer;
    ComPtr<ID3D12Resource> m_vbUpload;       // 临时 Upload Buffer
    ComPtr<ID3D12Resource> m_ibUpload;
    D3D12_VERTEX_BUFFER_VIEW m_vbv{};
    D3D12_INDEX_BUFFER_VIEW  m_ibv{};
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    bool m_useIndex = false;
};
