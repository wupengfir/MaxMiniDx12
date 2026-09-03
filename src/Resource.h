#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>


//#include "context.h"
#include "Constants.h"
#include "Log/Logs.h"
using Microsoft::WRL::ComPtr;

class ResourceUtils
{
public:

	static bool IsPowerOfTwo(uint32_t v)
	{
		return v != 0 && (v & (v - 1)) == 0;
	}

	static UINT AlignToUpperPo2(UINT value,UINT align)
	{
		if (!IsPowerOfTwo(align))
			assert("not power of 2");
		return (value + align - 1) & ~(align - 1);
	}
};

class GraphicsDevice;
class Resource
{
protected:
	D3D12_RESOURCE_DESC m_desc = {};
public :
	static GraphicsDevice* m_device;
	Resource(){}
	ID3D12Device* GetDevice() const;// { return m_device->DxDevice(); }
	Resource(const Resource&) = delete;
	Resource(Resource&&) noexcept = default;
	virtual ~Resource() = default;
	virtual ID3D12Resource* GetResource() { return nullptr; }
	virtual D3D12_RESOURCE_STATES GetState() { return D3D12_RESOURCE_STATE_COMMON; }
	virtual D3D12_RESOURCE_DESC GetResourceDesc() { return m_desc; }
};

class Buffer : public Resource
{
public :
	Buffer(){}
	Buffer(Buffer&&) = default;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const = 0;
	virtual size_t GetSize() const = 0;
	virtual ~Buffer() = default;
};


class DescriptorHeap : public Resource
{
protected :

	struct DescPtr
	{
		DescriptorHeap* heap;
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
	};

	ComPtr<ID3D12DescriptorHeap> m_heap;
	D3D12_DESCRIPTOR_HEAP_DESC m_desc;
	inline static std::unordered_map<ID3D12Resource*, DescPtr> m_DescResourceMap = {};
	inline static std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> m_NameHandleMap = {};
public :
				class ViewDesc
				{
				public:
					D3D12_RENDER_TARGET_VIEW_DESC* RtView;
					D3D12_DEPTH_STENCIL_VIEW_DESC* DsView;
					D3D12_SHADER_RESOURCE_VIEW_DESC* SrView;		
					ViewDesc(D3D12_RENDER_TARGET_VIEW_DESC* view) : RtView(view){}
					ViewDesc(D3D12_DEPTH_STENCIL_VIEW_DESC* view) : DsView(view){}
					ViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC* view) : SrView(view){}
				};
	bool Created;
	size_t Size;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle;
	static std::unordered_map<ID3D12Resource*, DescPtr>& DescResourceMap() { return m_DescResourceMap; }
	static std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE>& NameHandleMap() { return m_NameHandleMap; }
	DescriptorHeap( D3D12_DESCRIPTOR_HEAP_DESC&& desc):m_desc(desc) 
	{
		if (FAILED(GetDevice()->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap))))
			Created = false;   
		Created = true;
		m_heap->SetName(L"DescriptorHeap" + m_desc.Type);
		Size = GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);
		Handle = m_heap->GetCPUDescriptorHandleForHeapStart();
	}

	void Clear() {Handle = m_heap->GetCPUDescriptorHandleForHeapStart();}
	virtual void CreateView(ID3D12Resource* resource,ViewDesc desc) = 0;

	DescriptorHeap(DescriptorHeap&&) = default;
	DescriptorHeap(const DescriptorHeap&) = delete;
	virtual ~DescriptorHeap()
	{
		m_heap.Reset();
	}
};


class RTVDescriptorHeap : public DescriptorHeap
{
public:

	RTVDescriptorHeap(UINT count) : DescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = count,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
		})
	{

	}

	void CreateView(ID3D12Resource* resource, ViewDesc desc) override;

};


class DSVDescriptorHeap : public DescriptorHeap
{
public:

	DSVDescriptorHeap(UINT count) : DescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = count,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
		})
	{

	}

	void CreateView(ID3D12Resource* resource, ViewDesc desc) override;
};

class SRVDescriptorHeap : public DescriptorHeap
{
public:

	SRVDescriptorHeap(UINT count) : DescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = count,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0
		})
	{

	}

	void CreateView(ID3D12Resource* resource, ViewDesc desc) override;
};


class CommandBufferPool;
class CommandBuffer
{
private:
	CommandBufferPool* m_pool;
public:
	ComPtr<ID3D12GraphicsCommandList> CmdList;
	CommandBuffer(CommandBufferPool* pool)
	{
		m_pool = pool;
	}

	void Release();

	~CommandBuffer();

};
class CommandBufferPool : Resource
{
	

private:
	std::vector<ComPtr<ID3D12CommandAllocator>> m_allocators;
	//std::vector<ComPtr<CommandBuffer>> m_cmdListPool;
	std::vector<ComPtr<ID3D12GraphicsCommandList>> m_freeCmdListPool;

	UINT m_threadCount;
	UINT m_frameBufferCount;
	UINT m_gap;
public:
	enum class Type
	{
		DIRECT = 0,
		COMPUTE = 1,
		COPY = 2
	};

	CommandBufferPool(UINT threadCount,UINT frameBufferCount = FRAME_BACKBUFFER_COUNT):m_allocators()
	{
		m_allocators.resize(threadCount * frameBufferCount * QUEUE_TYPES);
		m_threadCount = threadCount;
		m_frameBufferCount = frameBufferCount;
		m_gap = frameBufferCount * QUEUE_TYPES;
		Init();
    }
	
	void ResetAllocators(UINT threadCount,UINT frameBufferCount)
	{
		//不要用.去调用reset。。。。。。。。
		m_allocators[threadCount * m_gap + frameBufferCount * QUEUE_TYPES + 0]->Reset();
		m_allocators[threadCount * m_gap + frameBufferCount * QUEUE_TYPES + 1]->Reset();
		m_allocators[threadCount * m_gap + frameBufferCount * QUEUE_TYPES + 2]->Reset();
	}

	bool Init()
	{
		for (size_t t = 0; t < m_threadCount; t++)
		{
			for (uint32_t i = 0; i < FRAME_BACKBUFFER_COUNT; ++i) {
				if (FAILED(GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_allocators[t*m_gap + i * QUEUE_TYPES + 0])))) {
					return  false;
				}
				if (FAILED(GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_allocators[t*m_gap + i * QUEUE_TYPES + 1])))) {
					return  false;
				}
				if (FAILED(GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_allocators[t*m_gap + i * QUEUE_TYPES + 2])))) {
					return  false;
				}
			}
		}

	}

	void FreeCommandBuffer(ComPtr<ID3D12GraphicsCommandList> buffer)
	{
		m_freeCmdListPool.push_back(buffer);
	}

	CommandBuffer AcquireCommandList(UINT threadID, UINT frameIndex, Type type);
	
	//~CommandBufferPool()
	//{
	//	for(ID3D12GraphicsCommandList* buffer:m_freeCmdListPool)
	//	{
	//		delete buffer;
	//	}
	//}
};

//class HeapManager
//{
//private:
//	std::vector<ComPtr<ID3D12Heap>> m_defaultHeaps;
//	std::vector<ComPtr<ID3D12Heap>> m_uploadHeaps;
//	std::vector<ComPtr<ID3D12Heap>> m_readbackHeaps;
//public:
//	ID3D12Heap* GetHeap
//};

class TextureBuffer : public Buffer
{
protected :
	ComPtr<ID3D12Resource> m_texture;

public:
	UINT Width;
	UINT Height;
	UINT8 Dimension;
	UINT8 Depth;
	UINT8 MipLevels;
	UINT8 MsaaCount;
	std::wstring Name;
	DXGI_FORMAT Format;
	
	TextureBuffer()
	{
		Dimension = 2;
		Depth = 1;
		MipLevels = 0;
		MsaaCount = 1;
	}

	ID3D12Resource* GetTexture()
	{
		return m_texture.Get();
	}

	virtual void CreateTexture()
	{
		m_desc.Dimension = Dimension == 2?D3D12_RESOURCE_DIMENSION_TEXTURE2D:D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		m_desc.Width = Width;
		m_desc.Height = Height;
		m_desc.DepthOrArraySize = Depth;
		m_desc.MipLevels = MipLevels;
		m_desc.Format = Format;
		m_desc.SampleDesc.Count = MsaaCount;
		m_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		m_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES defaultHeap{};
		defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
		defaultHeap.CreationNodeMask = 1;
		defaultHeap.VisibleNodeMask = 1;

		GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &m_desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&m_texture));
		
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override
	{
		if (m_texture)
			return m_texture->GetGPUVirtualAddress();
		return 0;
	}
	size_t GetSize() const override
	{
		return 0;
	}
};

class DepthTextureBuffer : public TextureBuffer
{
public :
	void CreateTexture() override;
	
};
