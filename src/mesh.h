#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

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
