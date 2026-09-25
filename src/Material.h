#pragma once
#include "Resource.h"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <unordered_set>
static void ShowError(const char* msg) {
    OutputDebugStringA(msg);
    MessageBoxA(nullptr, msg, "Shader Error", MB_ICONERROR);
}

//enum class ResourceType
//{
//	VERTEX,
//    PIXEL,
//    ALL
//};

class ShaderResourceParam
{
public :
	D3D12_SHADER_VISIBILITY visibility;
    std::string name;
	D3D12_SHADER_BUFFER_DESC bufferDesc;
	D3D12_SHADER_INPUT_BIND_DESC bindDesc;
	std::vector<D3D12_SHADER_VARIABLE_DESC> bufferDataDescs;
    std::vector<std::string> bufferDataDescNames;
    
    UINT alignedCBufferSize = 0;
    uint8_t* AlignedConstantBuffer = nullptr;
    ~ShaderResourceParam()
    {
        delete[] AlignedConstantBuffer;
    }
};

class ComputeShader
{
private:
	std::string m_path;
    std::string m_content;
    ComPtr<ID3DBlob> m_csBlob;
    std::vector<ShaderResourceParam> m_shaderResourceParams;
public:
	ComputeShader(const std::string&& path) :m_path(path) {}
    ComputeShader(const std::string& path) :m_path(path) {}

    const std::string& GetPath() { return m_path; };
    const std::string& GetContent() { return m_content; };
    ComPtr<ID3DBlob>& GetCsBlob() { return m_csBlob; }
    std::vector<ShaderResourceParam>& GetShaderResourceParams() { return m_shaderResourceParams; };

    void ReflectShader(ID3D12ShaderReflection* reflection,D3D12_SHADER_VISIBILITY visibility)
    {
        UINT alignedCBufferSize = 0;
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
	    D3D12_SHADER_DESC shaderDesc;
	    reflection->GetDesc(&shaderDesc);



	    for (size_t i = 0; i < shaderDesc.ConstantBuffers; i++)
	    {
            ShaderResourceParam cbvParam;
            cbvParam.visibility = visibility;            
		    ID3D12ShaderReflectionConstantBuffer* pCBuffer = reflection->GetConstantBufferByIndex(i);// 遍历 cbuffer 内部的每个变量 (你的 float, float4, matrix)
		    D3D12_SHADER_BUFFER_DESC cbufferDesc;
		    pCBuffer->GetDesc(&cbufferDesc); 
            
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflection->GetResourceBindingDescByName(cbufferDesc.Name,&bindDesc);
            if (bindDesc.Type != D3D_SIT_CBUFFER)continue;
            cbvParam.bindDesc = bindDesc;
            cbvParam.bufferDesc = cbufferDesc;
            cbvParam.name = cbufferDesc.Name;
            
		    for (UINT j = 0; j < cbufferDesc.Variables; j++)
		    {
			    ID3D12ShaderReflectionVariable* pVariable = pCBuffer->GetVariableByIndex(j);        
			    D3D12_SHADER_VARIABLE_DESC varDesc;
			    pVariable->GetDesc(&varDesc);
                alignedCBufferSize = std::max(alignedCBufferSize, varDesc.Size + varDesc.StartOffset);
                cbvParam.bufferDataDescs.push_back(varDesc);
                cbvParam.bufferDataDescNames.push_back(varDesc.Name);                
		    }
            cbvParam.alignedCBufferSize = alignedCBufferSize;
            m_shaderResourceParams.push_back(cbvParam);

	    }

        for (UINT i = 0; i < shaderDesc.BoundResources; i++)
        {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflection->GetResourceBindingDesc(i, &bindDesc);
            if (bindDesc.Type == D3D_SIT_CBUFFER)continue;
            ShaderResourceParam srvParam;
            srvParam.visibility = visibility;            
            srvParam.name = bindDesc.Name;
            srvParam.bindDesc = bindDesc;
            m_shaderResourceParams.push_back(srvParam);

        }
    }

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

	    if (m_content.empty())
		    return false;
	    if (!(Compile(m_content, GetPath().c_str(), "CSMain", "cs_5_1", GetCsBlob())))
	    {
		    return false;
	    }

        //  反射shader

        ComPtr<ID3D12ShaderReflection> reflection;
	    ID3DBlob* blob = GetCsBlob().Get();	    
	    D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),IID_PPV_ARGS(&reflection));
        ReflectShader(reflection.Get(),D3D12_SHADER_VISIBILITY_ALL);
        return true;
    }

	bool Compile(const std::string& source, const char* filename,
    const char* entryPoint, const char* target,ComPtr<ID3DBlob>& blob) {
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(source.c_str(), source.size(), filename,
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, 0, 0, &blob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) ShowError((char*)errorBlob->GetBufferPointer());
        return false;
    }
    return true;
}
};

class Shader
{
private:
	std::string m_path;
    std::string m_content;
    ComPtr<ID3DBlob> m_vsBlob;
    ComPtr<ID3DBlob> m_psBlob;
    std::vector<D3D12_SIGNATURE_PARAMETER_DESC> m_vertexInputs;
    std::vector<ShaderResourceParam> m_shaderResourceParams;
public:
	Shader(const std::string&& path) :m_path(path) {}
    
    std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& GetVertexInputs() { return m_vertexInputs; };
    std::vector<ShaderResourceParam>& GetShaderResourceParams() { return m_shaderResourceParams; };
    const std::string& GetPath() { return m_path; };
    const std::string& GetContent() { return m_content; };
    ComPtr<ID3DBlob>& GetVsBlob() { return m_vsBlob; }
    ComPtr<ID3DBlob>& GetPsBlob() { return m_psBlob; }

    void ReflectShader(ID3D12ShaderReflection* reflection,std::unordered_set<std::string>& deDoubleSet,D3D12_SHADER_VISIBILITY visibility)
    {
        UINT alignedCBufferSize = 0;
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
	    D3D12_SHADER_DESC shaderDesc;
	    reflection->GetDesc(&shaderDesc);

        //顶点shader拿顶点结构
        if (visibility == D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX)
        {
            D3D12_SIGNATURE_PARAMETER_DESC vertexinput;
            int index = 0;
            HRESULT hr = S_OK;
            while (SUCCEEDED(hr))
            {
                hr = reflection->GetInputParameterDesc(index++, &vertexinput);
                if(SUCCEEDED(hr))
                    GetVertexInputs().push_back(vertexinput);
            }            
        }

	    for (size_t i = 0; i < shaderDesc.ConstantBuffers; i++)
	    {
            ShaderResourceParam cbvParam;
            cbvParam.visibility = visibility;            
		    ID3D12ShaderReflectionConstantBuffer* pCBuffer = reflection->GetConstantBufferByIndex(i);// 遍历 cbuffer 内部的每个变量 (你的 float, float4, matrix)
		    D3D12_SHADER_BUFFER_DESC cbufferDesc;
		    pCBuffer->GetDesc(&cbufferDesc); 
            
            if (deDoubleSet.contains(cbufferDesc.Name))
            {
                for (auto& var:m_shaderResourceParams)
                {
                    if (var.name == cbufferDesc.Name)
                    {
                        var.visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
                        break;
                    }
                }
                continue;
            }
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflection->GetResourceBindingDescByName(cbufferDesc.Name,&bindDesc);
            cbvParam.bindDesc = bindDesc;
            cbvParam.bufferDesc = cbufferDesc;
            cbvParam.name = cbufferDesc.Name;
            
		    for (UINT j = 0; j < cbufferDesc.Variables; j++)
		    {
			    ID3D12ShaderReflectionVariable* pVariable = pCBuffer->GetVariableByIndex(j);        
			    D3D12_SHADER_VARIABLE_DESC varDesc;
			    pVariable->GetDesc(&varDesc);
                alignedCBufferSize = std::max(alignedCBufferSize, varDesc.Size + varDesc.StartOffset);
                cbvParam.bufferDataDescs.push_back(varDesc);
                cbvParam.bufferDataDescNames.push_back(varDesc.Name);                
		    }
            cbvParam.alignedCBufferSize = alignedCBufferSize;
            m_shaderResourceParams.push_back(cbvParam);
            deDoubleSet.insert(cbvParam.name);
	    }

        for (UINT i = 0; i < shaderDesc.BoundResources; i++)
        {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflection->GetResourceBindingDesc(i, &bindDesc);
            if (bindDesc.Type == D3D_SIT_CBUFFER)continue;
            if (deDoubleSet.contains(bindDesc.Name))
            {
                for (auto& var:m_shaderResourceParams)
                {
                    if (var.name == bindDesc.Name)
                    {
                        var.visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
                        break;
                    }
                }
                continue;
            }
            ShaderResourceParam srvParam;
            srvParam.visibility = visibility;            
            srvParam.name = bindDesc.Name;
            srvParam.bindDesc = bindDesc;
            m_shaderResourceParams.push_back(srvParam);
            deDoubleSet.insert(srvParam.name);
        }
    }

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

	    if (m_content.empty())
		    return false;
	    if (!(Compile(m_content, GetPath().c_str(), "VS", "vs_5_0", GetVsBlob())))
	    {
		    return false;
	    }
	    if (!(Compile(m_content, GetPath().c_str(), "PS", "ps_5_0", GetPsBlob())))
	    {
		    return false;
	    }

        //  反射shader
        std::unordered_set<std::string> deDoubleSet{};
        ComPtr<ID3D12ShaderReflection> reflection;
	    ID3DBlob* blob = GetVsBlob().Get();	    
	    D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),IID_PPV_ARGS(&reflection));
        ReflectShader(reflection.Get(),deDoubleSet,D3D12_SHADER_VISIBILITY_VERTEX);
        blob = GetPsBlob().Get();	    
	    D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),IID_PPV_ARGS(&reflection));
        ReflectShader(reflection.Get(),deDoubleSet,D3D12_SHADER_VISIBILITY_PIXEL);

        //alignedCBufferSize = (alignedCBufferSize + 255) & ~(255);

        return true;
    }

	bool Compile(const std::string& source, const char* filename,
    const char* entryPoint, const char* target,ComPtr<ID3DBlob>& blob) {
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(source.c_str(), source.size(), filename,
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, 0, 0, &blob, &errorBlob);
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

enum class MaterialPropertyType
{
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    INT,
    INT1,
    INT2,
    INT3,
    FLOAT4x4,
    Texture,
    Buffer
};

struct MaterialProperty
{
    MaterialPropertyType type = MaterialPropertyType::FLOAT;
    UINT offset = 0;
    UINT size = 0;
    //后面来优化用一个buffer装
    uint8_t* data = nullptr;
    TextureBuffer* texture = nullptr;
    StructureBuffer* buffer = nullptr;

    MaterialProperty()
    {

    }

    MaterialProperty(MaterialProperty&& other) noexcept
    {
        type = other.type;
        offset = other.offset;
        size = other.size;
        data = other.data;
        texture = other.texture;
        buffer = other.buffer;
        other.data = nullptr;
    }
    MaterialProperty& operator=(MaterialProperty&& other) noexcept
    {

        if (this != &other)
        {
            if(data)
                delete[] data;
            type = other.type;
            offset = other.offset;
            size = other.size;
            data = other.data;
            texture = other.texture;
            buffer = other.buffer;
            other.data = nullptr;
        }
        return *this;
    }

    MaterialProperty& operator=(MaterialProperty& other) noexcept
    {

        if (this != &other)
        {
            if(data)
                delete[] data;
             type = other.type;
            offset = other.offset;
            size = other.size;
            data = other.data;
            texture = other.texture;
            buffer = other.buffer;
            other.data = nullptr;
        }
        return *this;
    }

    ~MaterialProperty()
    {
        if(data)
            delete[] data;
    }
};

class Material : public Resource
{
protected:
	ComPtr<ID3D12RootSignature> m_rootSig;
    ComPtr<ID3D12PipelineState> m_pso;
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
    Shader* m_shader = nullptr;
    ComputeShader* m_computeShader = nullptr;
    std::unordered_map<std::string, MaterialProperty> m_MaterialProperties{};
    inline static std::unordered_map<std::string, MaterialProperty> m_GlobalProperties{};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc{};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_computePsoDesc{};
    std::vector<uint8_t> m_buffer{};
    ComPtr<ID3D12Resource> m_bufferResource;
public:
    D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_FRONT;
    D3D12_COMPARISON_FUNC DepthTest = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    bool DepthEnable = true;
    bool BlendEnable = false;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& PsoDesc() { return m_psoDesc; }
    D3D12_COMPUTE_PIPELINE_STATE_DESC& ComputePsoDesc() { return m_computePsoDesc; }
	ComPtr<ID3D12RootSignature>& GetRootSignature() { return m_rootSig; }
	ComPtr<ID3D12PipelineState>& GetPso() { return m_pso; }
    void SetShader(Shader* shader) { m_shader = shader; m_pso.Reset(); }
    void SetComputeShader(ComputeShader* shader) { m_computeShader = shader; m_pso.Reset(); }
    bool Create();
    ID3D12Resource* BufferResource() { return m_bufferResource.Get(); }
    bool RecreateAlignedConstantBuffer = false;
    uint8_t* MaterialData() { return m_buffer.data(); }
    
    Shader* GetShader()
    {
        return m_shader;
    }
    ComputeShader* GetComputeShader()
    {
        return m_computeShader;
    }
    MaterialProperty* FindProperty(const std::string& name)
    {
        auto it = m_MaterialProperties.find(name);
        if (it != m_MaterialProperties.end())
        {
            return &(it->second);
        }
        it = m_GlobalProperties.find(name);
        if (it != m_GlobalProperties.end())
        {
            return &(it->second);
        }
        return nullptr;
    }

    void SetFloat(const std::string name, float data)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::FLOAT;
        property.size = sizeof(float);
        property.offset = m_buffer.size();

        if (property.data)
            delete property.data;
        property.data = new uint8_t[property.size];
        memcpy(property.data, &data, property.size);

        /*if (m_buffer.capacity() < (m_buffer.size() + property.size))
        {
            m_buffer.reserve((m_buffer.size() + property.size)*2);
        }
        memcpy(m_buffer.data() + property.offset, &data, property.size);*/
        m_MaterialProperties[name] = std::move(property);
    }

    void SetFloat4(const std::string name, XMFLOAT4 data)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::FLOAT4;
        property.size = sizeof(XMFLOAT4);
        property.offset = m_buffer.size();
        if (property.data)
            delete property.data;
        property.data = new uint8_t[property.size];
        memcpy(property.data, &data, property.size);
        /*if (m_buffer.capacity() < (m_buffer.size() + property.size))
        {
            m_buffer.reserve((m_buffer.size() + property.size)*2);
        }
        memcpy(m_buffer.data() + property.offset, &data, property.size);*/
         m_MaterialProperties[name] = std::move(property);
        //printf("x origin is :  %f\n",*((float*)(m_MaterialProperties[name].data)));
    }

    template<typename T>
    void SetValue(MaterialPropertyType type,const std::string name, T data)
    {
        MaterialProperty property{};
        property.type = type;
        property.size = sizeof(T);
        property.offset = m_buffer.size();
        if (property.data)
            delete[] property.data;
        property.data = new uint8_t[property.size];
        memcpy(property.data, &data, property.size);
        m_MaterialProperties[name] = std::move(property);

    }

    template<typename T>
    inline static void SetGlobalValue(MaterialPropertyType type,const std::string name, T data)
    {
        MaterialProperty property{};
        property.type = type;
        property.size = sizeof(T);
        property.offset = m_buffer.size();
        if (property.data)
            delete[] property.data;
        property.data = new uint8_t[property.size];
        memcpy(property.data, &data, property.size);
        m_GlobalProperties[name] = std::move(property);

    }

    void SetTexture(const std::string name, TextureBuffer* texture,ID3D12GraphicsCommandList* cmdList = nullptr)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::Texture;
        property.texture = texture;
         m_MaterialProperties[name] = property;
         
    }

    void SetBuffer(const std::string name, StructureBuffer* buffer,ID3D12GraphicsCommandList* cmdList = nullptr)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::Buffer;
        property.buffer = buffer;
         m_MaterialProperties[name] = property;
         
    }

    inline static void SetGlobalTexture(const std::string name, TextureBuffer* texture,ID3D12GraphicsCommandList* cmdList = nullptr)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::Texture;
        property.texture = texture;
         m_GlobalProperties[name] = property;
        
    }

    inline static void SetGlobalBuffer(const std::string name, StructureBuffer* buffer,ID3D12GraphicsCommandList* cmdList = nullptr)
    {
        MaterialProperty property{};
        property.type = MaterialPropertyType::Buffer;
        property.buffer = buffer;
         m_GlobalProperties[name] = property;
        
    }

};