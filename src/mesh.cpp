#include "mesh.h"
#include <cstring>

void Mesh::Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const Vertex* vertices, uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount) {

    m_vertexCount = vertexCount;
    m_indexCount = indexCount;
    m_useIndex = (indices != nullptr && indexCount > 0);

    uint32_t vbSize = vertexCount * sizeof(Vertex);

    // 两种 Heap 的属性描述
    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeap.CreationNodeMask = 1;
    defaultHeap.VisibleNodeMask = 1;

    D3D12_HEAP_PROPERTIES uploadHeap{};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeap.CreationNodeMask = 1;
    uploadHeap.VisibleNodeMask = 1;

    // 缓冲描述（Width = 字节数，Buffer 类型）
    D3D12_RESOURCE_DESC bufDesc{};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // ====== 顶点缓冲 ======

    // 1. 创建 Default Buffer（初始状态 COPY_DEST，准备接收拷贝）
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &bufDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_vertexBuffer));

    // 2. 创建 Upload Buffer（初始状态 GENERIC_READ，CPU 可写）
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_vbUpload));

    // 3. Map：获得 Upload Buffer 的 CPU 可写指针，memcpy 数据
    D3D12_RANGE readRange{ 0, 0 };  // CPU 不会读这块内存，读范围设为 0（性能提示）
    void* pData = nullptr;
    m_vbUpload->Map(0, &readRange, &pData);
    memcpy(pData, vertices, vbSize);
    m_vbUpload->Unmap(0, nullptr);

    // 4. GPU 拷贝命令：Upload Buffer → Default Buffer
    cmdList->CopyBufferRegion(m_vertexBuffer.Get(), 0, m_vbUpload.Get(), 0, vbSize);

    // 5. Resource Barrier：COPY_DEST → VERTEX_AND_CONSTANT_BUFFER
    //    拷贝完了，这块缓冲接下来要被当作顶点缓冲读，必须转换状态
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_vertexBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // 6. 创建 Vertex Buffer View（不是资源，只是一个描述结构体，告诉 IA 怎么读顶点缓冲）
    m_vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vbv.StrideInBytes = sizeof(Vertex);
    m_vbv.SizeInBytes = vbSize;

    // ====== 索引缓冲（同样的流程） ======
    if (m_useIndex) {
        uint32_t ibSize = indexCount * sizeof(uint32_t);
        bufDesc.Width = ibSize;

        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
            &bufDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_indexBuffer));

        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
            &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_ibUpload));

        m_ibUpload->Map(0, &readRange, &pData);
        memcpy(pData, indices, ibSize);
        m_ibUpload->Unmap(0, nullptr);

        cmdList->CopyBufferRegion(m_indexBuffer.Get(), 0, m_ibUpload.Get(), 0, ibSize);

        barrier.Transition.pResource = m_indexBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        cmdList->ResourceBarrier(1, &barrier);

        m_ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_ibv.SizeInBytes = ibSize;
        m_ibv.Format = DXGI_FORMAT_R32_UINT;  // 32 位索引
    }
}

void Mesh::ReleaseTemporary() {
    m_vbUpload.Reset();
    m_ibUpload.Reset();
}

void Mesh::Draw(ID3D12GraphicsCommandList* cmdList) const {
    cmdList->IASetVertexBuffers(0, 1, &m_vbv);
    if (m_useIndex) {
        cmdList->IASetIndexBuffer(&m_ibv);
        cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }
    else {
        cmdList->DrawInstanced(m_vertexCount, 1, 0, 0);
    }
}
