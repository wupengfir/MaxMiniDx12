#pragma once
#include <d3d12.h>
#include <vector>
#include <functional>
class DescPtr;
class Material;
class StructureBuffer;
class TextureBuffer;
class CubemapRenderTextureBuffer;
class RenderPass
{
protected:
	TextureBuffer* m_Color;	
	TextureBuffer* m_Depth;
	UINT m_ColorRtCount;
	//std::vector<D3D12_RESOURCE_DESC> m_ColorDescs{};
	//D3D12_RESOURCE_DESC m_DepthDesc{};
public:
	UINT cameraIndex = 0;
	void SetRenderTaget(TextureBuffer* rt, TextureBuffer* depth);
	virtual void ExecutePass(ID3D12CommandQueue* queue) = 0;
};

class OpaquePass : public RenderPass
{
public:
	inline static ID3D12Resource* ColorAttachment = nullptr;
	void ExecutePass(ID3D12CommandQueue* queue) override ;
};

class PostProcessPass : public RenderPass
{
private : 
	Material* m_postprocessMaterial;
public:
	void SetPostMaterial(Material* mat) { m_postprocessMaterial = mat; }
	void ExecutePass(ID3D12CommandQueue* queue) override ;
};

class CubemapConvolovePass : public RenderPass
{
private :	
	CubemapRenderTextureBuffer* m_diffuseIrradiance;
	CubemapRenderTextureBuffer* m_reflectIrradiance;
	Material* m_cubeMapConvoloveMaterial;
	Material* m_reflectConvoloveMaterial;
public:
	CubemapConvolovePass();
	void SetConvoloveMaterial(Material* mat) { m_cubeMapConvoloveMaterial = mat; }
	void SetReflectConvoloveMaterial(Material* mat) { m_reflectConvoloveMaterial = mat; }
	void ExecutePass(ID3D12CommandQueue* queue) override ;
	~CubemapConvolovePass()
	{
		if(m_diffuseIrradiance)
			delete m_diffuseIrradiance;
		if (m_reflectIrradiance)
			delete m_reflectIrradiance;
	}
};

class EnvironmetConvolovePass : public RenderPass
{
private :
	StructureBuffer* uavBuffer = nullptr;
	Material* m_convoloveMaterial;
public:
	void SetConvoloveMaterial(Material* mat) { m_convoloveMaterial = mat; }
	void ExecutePass(ID3D12CommandQueue* queue) override ;

	~EnvironmetConvolovePass()
	{
		delete uavBuffer;
	}

};

enum class ReadBackType
{
	Texture,
	UAVTexture,
	CBuffer,
	UAVBuffer
};

struct ReadBackFunction
{
	ReadBackType type;
	ID3D12Resource* resource;
	size_t size;
	std::function<void(UINT64)> callback;
};

class ReadBackPass : public RenderPass
{
private:

public:
	void ExecutePass(ID3D12CommandQueue* queue) override ;
	inline static std::vector<ReadBackFunction> asyncCallbacks{};
	inline static std::vector<ReadBackFunction> syncCallbacks{};
};