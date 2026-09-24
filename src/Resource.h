#pragma once

#include "Common.h"

#include "Constants.h"
#include "Log/Logs.h"

class ReadBackFunction;

namespace DirectX
{
    struct Image;
}
UINT GetBitsPerPixel(DXGI_FORMAT fmt);

void InitEngineResource();

inline UINT GetPixelBytes(DXGI_FORMAT fmt)
{
    return GetBitsPerPixel(fmt)/8U;
}


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
class Resource;
struct ResourceAndStatus
{
public:
	Resource* resource;
	D3D12_RESOURCE_STATES status;
};

class Context;
class Resource
{
protected:
	D3D12_RESOURCE_DESC m_desc = {};
public :
	static Context* m_device;
	inline static std::unordered_map<ID3D12Resource*, ResourceAndStatus> StatusMap{};
	inline static ResourceAndStatus& FindResourceAndStatus(ID3D12Resource* ptr)
	{
		ResourceAndStatus result{};
		auto status = Resource::StatusMap.find(ptr);
        if (status == Resource::StatusMap.end())
        {
            return result;
        }
		return status->second;
	}
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

class ConstantBufferHeap : public Buffer
{
private :
	ComPtr<ID3D12Resource> m_heap;
	UINT m_heapSize;
	void* m_basePointer;
	UINT64 m_offset;
public:
	void init(UINT size = 256*262144);//64M
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override
	{
		return m_heap?m_heap->GetGPUVirtualAddress():0;
	}

	size_t GetSize() const override
	{
		return m_heapSize;
	}

	UINT64 Offset()
	{
		return m_offset;
	}

	UINT64 WriteConstantBuffer(void* data,size_t size)
	{
		//todo: 需要判断之前的buffer有没有在用
		if (((m_offset + size + 255) & ~255) >= m_heapSize)
			m_offset = 0;
		memcpy(static_cast<uint8_t*>(m_basePointer) + m_offset, data, size);
		UINT64 temp = m_offset;
		m_offset += size;
		m_offset = (m_offset + 255) & ~255;
		return temp;
	}

	/*void* GetCpuBasePointer()
	{
		return m_basePointer;
	}*/

	~ConstantBufferHeap()
	{
		m_heap->Unmap(0,nullptr);
		m_basePointer = nullptr;
	}
};

class ReadbackBufferHeap : public Buffer
{
private :
	ComPtr<ID3D12Resource> m_heap;
	UINT m_heapSize;
	void* m_basePointer = nullptr;
	UINT64 m_offset;
public:
	void init(UINT size = 256*262144);//64M

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override
	{
		return m_heap?m_heap->GetGPUVirtualAddress():0;
	}

	void* GetPointer(UINT64 offset)
	{
		return (uint8_t*)m_basePointer + offset;
	}

	size_t GetSize() const override
	{
		return m_heapSize;
	}

	UINT64 Offset()
	{
		return m_offset;
	}

	
	UINT64 CopyResourceSync(ReadBackFunction* data,ID3D12GraphicsCommandList* cmdlist);

	~ReadbackBufferHeap()
	{
		m_heap->Unmap(0,nullptr);
		m_basePointer = nullptr;
	}
};

class DescriptorHeap;
struct DescPtr
{
	DescriptorHeap* heap;
	ID3D12Resource* resource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle;
};

class DescriptorHeap : public Resource
{
protected :



	ComPtr<ID3D12DescriptorHeap> m_heap;
	D3D12_DESCRIPTOR_HEAP_DESC m_desc;
	//inline static std::unordered_map<ID3D12Resource*, DescPtr> m_DescResourceMap = {};
	inline static std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> m_NameHandleMap = {};
public :
				class ViewDesc
				{
				public:
					union
					{
						D3D12_RENDER_TARGET_VIEW_DESC* RtView;
						D3D12_DEPTH_STENCIL_VIEW_DESC* DsView;
						D3D12_SHADER_RESOURCE_VIEW_DESC* SrView;
						D3D12_UNORDERED_ACCESS_VIEW_DESC* UaView;
					};
							
					ViewDesc(D3D12_RENDER_TARGET_VIEW_DESC* view) : RtView(view){}
					ViewDesc(D3D12_DEPTH_STENCIL_VIEW_DESC* view) : DsView(view){}
					ViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC* view) : SrView(view){}
					ViewDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC* view) : UaView(view){}
				};
	bool Created;
	size_t Size;
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	//static std::unordered_map<ID3D12Resource*, DescPtr>& DescResourceMap() { return m_DescResourceMap; }
	static std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE>& NameHandleMap() { return m_NameHandleMap; }
	DescriptorHeap( D3D12_DESCRIPTOR_HEAP_DESC&& desc):m_desc(desc) 
	{
		if (FAILED(GetDevice()->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap))))
			Created = false;   
		Created = true;
		m_heap->SetName(L"DescriptorHeap" + m_desc.Type);
		Size = GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);
		CpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		GpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
	}
	ID3D12DescriptorHeap* Heap() { return m_heap.Get(); }
	void Clear() {CpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();}
	virtual DescPtr CreateView(ID3D12Resource* resource,ViewDesc desc) = 0;

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

	DescPtr CreateView(ID3D12Resource* resource, ViewDesc desc) override;

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

	DescPtr CreateView(ID3D12Resource* resource, ViewDesc desc) override;
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

	DescPtr CreateView(ID3D12Resource* resource, ViewDesc desc) override;
	DescPtr CreateUAView(ID3D12Resource* resource, ViewDesc desc);
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

enum class ViewType
{
	SRV = 0,
	RTV,
	UAV,
	DSV,
	Count
};

class TextureBuffer : public Buffer
{
protected :
	ComPtr<ID3D12Resource> m_texture;
	ComPtr<ID3D12Resource> tempUpload;
public:
	
	UINT Width;
	UINT Height;
	UINT16 Dimension;
	UINT16 Depth;
	UINT16 MipLevels;
	UINT16 SubresourceCount;
	UINT16 MsaaCount;
	D3D12_SRV_DIMENSION SrvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	bool Srgb = false;
	std::unique_ptr<uint8_t> TextureDataPointer;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> CPUHandles;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GPUHandles;
	std::wstring Name;
	DXGI_FORMAT Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	
	TextureBuffer()
	{
		Dimension = 2;
		Depth = 1;
		SubresourceCount = 1;
		MipLevels = 1;
		MsaaCount = 1;
		CPUHandles.resize((int)ViewType::Count);
		GPUHandles.resize((int)ViewType::Count);
	}

	ID3D12Resource* GetTexture()
	{
		return m_texture.Get();
	}

	virtual void CreateTexture();	

	void UploadTexture(uint8_t* textureData,ID3D12GraphicsCommandList* cmdList);
	void UploadTexture(const Image* images, ID3D12GraphicsCommandList* cmdList);
	void ReleaseUploadBuffer() { tempUpload.Reset(); };
	void Release()
	{
		tempUpload.Reset();
		m_texture.Reset();
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
	static TextureBuffer& GetDefaultWhiteTex();
	static TextureBuffer& GetDefaultNormalTex();
};

class DepthTextureBuffer : public TextureBuffer
{
public :
	void CreateTexture() override;
	
};

class RenderTextureBuffer : public TextureBuffer
{
public :
	bool AllowMipmap = false;
	bool AllowUAV = false;
	void CreateTexture() override;
	
};

class CubemapRenderTextureBuffer : public RenderTextureBuffer
{
public:
	CubemapRenderTextureBuffer():RenderTextureBuffer()
	{
		Depth = 6;
		CPUHandles.resize((int)ViewType::Count*Depth);
		GPUHandles.resize((int)ViewType::Count*Depth);
	}
	void CreateTexture() override;
};

//class Texture2D : public TextureBuffer
//{
//public :
//	void LoadTexture();
//	void CreateTexture() override;
//	void UPloadTexture();
//};

class StructureBuffer : public Buffer
{
protected :
	ComPtr<ID3D12Resource> m_buffer;
	ComPtr<ID3D12Resource> tempUpload;
public:
	
	UINT Width;
	UINT Stride;
	bool ReadWrite = false;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> CPUHandles;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GPUHandles;
	std::wstring Name;
	
	StructureBuffer()
	{
		CPUHandles.resize((int)ViewType::Count);
		GPUHandles.resize((int)ViewType::Count);
	}

	ID3D12Resource* GetBuffer()
	{
		return m_buffer.Get();
	}

	void CreateBuffer();	

	void UploadBuffer(uint8_t* data,ID3D12GraphicsCommandList* cmdList);

	void ReleaseUploadBuffer() { tempUpload.Reset(); };
	void Release()
	{
		tempUpload.Reset();
		m_buffer.Reset();
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override
	{
		return m_buffer->GetGPUVirtualAddress();;
	}

	size_t GetSize() const override
	{
		return Width * Stride;
	}
};