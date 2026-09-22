#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <DirectXMath.h>
#include "Resource.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Mesh : public Resource
{
private:
    inline static std::vector<Mesh*> m_allMeshs{};
    bool selfDelete = true;
    Mesh() = default;
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
    struct IndicesOffset
    {
        UINT start;
        UINT size;
    };
    std::vector<XMFLOAT3> vertices;
    std::vector<UINT> indices;
    std::vector<XMFLOAT3> normal;
    std::vector<XMFLOAT4> tangent;
    std::vector<XMFLOAT4> color;
    std::vector<XMFLOAT4> uv0;
    std::vector<XMFLOAT4> uv1;
    std::vector<XMFLOAT4> uv2;
    std::vector<XMFLOAT4> uv3;
    std::vector<float> data;
    std::vector<IndicesOffset> IndicesOffsets{};
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

    static Mesh* Create()
    {
        auto m = new Mesh();
        m_allMeshs.push_back(m);
        return m;
    }

    static Mesh* Load(const std::string path)
    {
        auto m = new Mesh();
        m_allMeshs.push_back(m);
        m->LoadMesh(path);
        return m;
    }

    ~Mesh()
    {
        if (selfDelete)
        {
            auto it = std::find(m_allMeshs.begin(), m_allMeshs.end(), this);
            if (it != m_allMeshs.end())
            {
                m_allMeshs[it - m_allMeshs.begin()] = m_allMeshs[m_allMeshs.size() - 1];
            }
            m_allMeshs.erase(m_allMeshs.begin()+m_allMeshs.size()-1);
        }
    }

    static void ReleaseAllMeshs()
    {
        for (auto p : m_allMeshs)
        {
            p->selfDelete = false;
            delete p;
        }
    }

    void LoadMesh(const std::string& path)
    {
        Assimp::Importer importer;

        // 关键：禁用枢轴节点，避免 Assimp 插入 _$AssimpFbx$_ 中间节点
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        const unsigned int flags =
            aiProcess_Triangulate |              // 面转三角形
            aiProcess_JoinIdenticalVertices |    // 合并重复顶点
            aiProcess_PreTransformVertices |     // 烘焙节点变换到顶点坐标
            aiProcess_ConvertToLeftHanded;       // 转左手坐标系（DX 用）

        const aiScene* scene = importer.ReadFile(path, flags);

        if (scene == nullptr || scene->mNumMeshes == 0) {
            throw std::runtime_error(
                "Assimp 加载失败: " + path + " - " + importer.GetErrorString()
            );
        }

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            const aiMesh* mesh = scene->mMeshes[m];

            // 记录当前 mesh 的顶点在全局数组中的起始下标
            const uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            // 提取顶点属性
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
 
                const aiVector3D& pos = mesh->mVertices[v];
                vertices.push_back(XMFLOAT3(pos.x,pos.y,pos.z));

                if (mesh->HasVertexColors(0))
                {
                    const aiColor4D& col = mesh->mColors[0][v];
                    color.push_back(XMFLOAT4(col.r,col.g,col.b,col.a));
                }
                else
                {
                    color.push_back(XMFLOAT4(0,0,0,0));
                }

                if (mesh->HasNormals()) {
                    const aiVector3D& nor = mesh->mNormals[v];
                    normal.push_back(XMFLOAT3(nor.x,nor.y,nor.z));
                }
                else
                {
                    normal.push_back(XMFLOAT3(0,0,1));
                }

                if (mesh->HasTangentsAndBitangents()) {
                    const aiVector3D& nor = mesh->mTangents[v];
                    tangent.push_back(XMFLOAT4(nor.x,nor.y,nor.z,1));
                }
                else
                {
                    tangent.push_back(XMFLOAT4(1,0,0,1));
                }


                if (mesh->HasTextureCoords(0)) {
                    const aiVector3D& uv = mesh->mTextureCoords[0][v];
                    uv0.push_back(XMFLOAT4(uv.x,uv.y,0,0));
                }
                else
                {
                    uv0.push_back(XMFLOAT4{});
                }
            }
            IndicesOffsets.push_back({ (UINT)indices.size(), mesh->mNumFaces * mesh->mFaces[0].mNumIndices});
            // 提取索引（必须加上 baseVertex 偏移）
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                    indices.push_back(
                        baseVertex + static_cast<uint32_t>(face.mIndices[i])
                    );
                }
            }
    }

   }

};


