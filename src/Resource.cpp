#include "Resource.h"
#include "RenderPass.h"
#include <iostream>
#include <comdef.h>
#include "context.h"
#include <DirectXTex.h>
UINT GetBitsPerPixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
        return 8; //1字节

    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
        return 16; //2字节

    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return 32; //4字节

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return 64; //8字节

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128; //16字节

    // 深度模板格式
    case DXGI_FORMAT_D16_UNORM: return 16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return 32;
    case DXGI_FORMAT_D32_FLOAT: return 32;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 64;

    // BC压缩格式，返回0，单独处理块大小
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 0;

    default:
        return 0;
    }
}

Context* Resource::m_device = nullptr;

ID3D12Device* Resource::GetDevice() const { return m_device->DxDevice(); }

CommandBuffer::~CommandBuffer()
{
	m_pool->FreeCommandBuffer(this->CmdList);
}

//void CommandBuffer::Release()
//{
//	m_pool->FreeCommandBuffer(this->CmdList);
//}

DescPtr RTVDescriptorHeap::CreateView(ID3D12Resource* resource, ViewDesc desc)
{
	GetDevice()->CreateRenderTargetView(resource, desc.RtView, CpuHandle);
	DescPtr p{ this,resource,CpuHandle,GpuHandle};
	//m_DescResourceMap[resource] = p;
	CpuHandle.ptr += Size;
	GpuHandle.ptr += Size;
	return p;
}

DescPtr DSVDescriptorHeap::CreateView(ID3D12Resource* resource,ViewDesc desc)
{
	m_device->DxDevice()->CreateDepthStencilView(resource, desc.DsView, CpuHandle);
	DescPtr p{ this,resource,CpuHandle,GpuHandle};
	//m_DescResourceMap[resource] = p;
	CpuHandle.ptr += Size;
	GpuHandle.ptr += Size;
	return p;
}

DescPtr SRVDescriptorHeap::CreateView(ID3D12Resource* resource,ViewDesc desc) 
{
	m_device->DxDevice()->CreateShaderResourceView(resource, desc.SrView, CpuHandle);
	DescPtr p{ this,resource,CpuHandle,GpuHandle};
	//m_DescResourceMap[resource] = p;
	CpuHandle.ptr += Size;
	GpuHandle.ptr += Size;
	return p;
}

DescPtr SRVDescriptorHeap::CreateUAView(ID3D12Resource* resource, ViewDesc desc)
{
	m_device->DxDevice()->CreateUnorderedAccessView(resource,nullptr, desc.UaView, CpuHandle);
	DescPtr p{ this,resource,CpuHandle,GpuHandle};
	//m_DescResourceMap[resource] = p;
	CpuHandle.ptr += Size;
	GpuHandle.ptr += Size;
	return p;
}

	CommandBuffer CommandBufferPool::AcquireCommandList(UINT threadID,UINT frameIndex,Type type)
	{
		ID3D12CommandAllocator* alloctor = m_allocators[threadID * m_gap + frameIndex * QUEUE_TYPES + static_cast<UINT>(type)].Get();
		//CommandBuffer cmdList(this);
		CommandBuffer commandBuffer(this);

		if (m_freeCmdListPool.size() > 0)
		{
			commandBuffer.CmdList = m_freeCmdListPool[m_freeCmdListPool.size() - 1];
			m_freeCmdListPool.erase(m_freeCmdListPool.begin() + m_freeCmdListPool.size() - 1);
			
		}
		else
		{
			D3D12_COMMAND_LIST_TYPE cmdtype = D3D12_COMMAND_LIST_TYPE_DIRECT;
			switch (type)
			{
				case CommandBufferPool::Type::DIRECT:
					cmdtype = D3D12_COMMAND_LIST_TYPE_DIRECT;
					break;
				case CommandBufferPool::Type::COMPUTE:
					cmdtype = D3D12_COMMAND_LIST_TYPE_COMPUTE;
				break;
				case CommandBufferPool::Type::COPY:
					cmdtype = D3D12_COMMAND_LIST_TYPE_COPY;
				break;
			}
			HRESULT hr = m_device->DxDevice()->CreateCommandList(0, cmdtype,
				alloctor, nullptr, IID_PPV_ARGS(&(commandBuffer.CmdList)));
			if (FAILED(hr)) {
				_com_error err(hr);
				std::wcout << L"!!!!!错误: " << err.ErrorMessage() << std::endl;
				assert(false);
			}
			commandBuffer.CmdList->Close();
		}

		
		commandBuffer.CmdList->Reset(alloctor, nullptr);
		return commandBuffer;
		//m_cmdListPool.push_back(cmdList);
	}

void DepthTextureBuffer::CreateTexture()
	{
		m_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		m_desc.Width = Width;
		m_desc.Height = Height;
		m_desc.DepthOrArraySize = Depth;
		m_desc.MipLevels = 1;
		m_desc.Format = DXGI_FORMAT_D32_FLOAT;
		m_desc.SampleDesc.Count = MsaaCount;
		m_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		m_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES defaultHeap{};
		defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
		defaultHeap.CreationNodeMask = 1;
		defaultHeap.VisibleNodeMask = 1;

		    // 创建时指定 CLEAR_VALUE 和初始状态 DEPTH_WRITE
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;  // 清深度为最远

		GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &m_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&m_texture));
		
		{
			//depth view
			D3D12_DEPTH_STENCIL_VIEW_DESC sdDesc = {};
			sdDesc.Format = DXGI_FORMAT_D32_FLOAT;
			sdDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			sdDesc.Texture2D.MipSlice = 0;
			DescriptorHeap::ViewDesc viewdesc(&sdDesc);
			DescPtr descPtr = m_device->DsvHeap()->CreateView(m_texture.Get(), viewdesc);
			CPUHandles[(int)ViewType::DSV] = descPtr.cpuhandle;
			GPUHandles[(int)ViewType::DSV] = descPtr.gpuhandle;
		}
		{
			//srv view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = SrvDimension;
			srvDesc.Texture2D.MostDetailedMip = 0;      // 从最详细的Mip 0开始
			srvDesc.Texture2D.MipLevels = -1;           // 使用从 MostDetailedMip 开始的所有剩余Mip层级
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 不对LOD进行限制
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			DescriptorHeap::ViewDesc viewdesc(&srvDesc);
			DescPtr descPtr = m_device->SrvHeap()->CreateView(m_texture.Get(),viewdesc);
			CPUHandles[(int)ViewType::SRV] = descPtr.cpuhandle;
			GPUHandles[(int)ViewType::SRV] = descPtr.gpuhandle;
		}
		
		

		m_texture->SetName(L"DepthBuffer");
		StatusMap[GetTexture()] = {this,D3D12_RESOURCE_STATE_DEPTH_WRITE};
	}


void RenderTextureBuffer::CreateTexture()
	{
		m_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		m_desc.Width = Width;
		m_desc.Height = Height;
		m_desc.DepthOrArraySize = Depth;
		m_desc.MipLevels = AllowMipmap?0:1;
		m_desc.Format = Format;
		m_desc.SampleDesc.Count = MsaaCount;
		m_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		m_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if(AllowUAV)
			m_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		D3D12_HEAP_PROPERTIES defaultHeap{};
		defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
		defaultHeap.CreationNodeMask = 1;
		defaultHeap.VisibleNodeMask = 1;


		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = Format;
		clearValue.Color[0] = 0; 
		clearValue.Color[1] = 0;
		clearValue.Color[2] = 0;
		clearValue.Color[3] = 0;

		GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &m_desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
        IID_PPV_ARGS(&m_texture));

		

		{
			//rt view
			D3D12_RENDER_TARGET_VIEW_DESC sdDesc = {};
			sdDesc.Format = Format;
			sdDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			sdDesc.Texture2D.MipSlice = 0;
			DescriptorHeap::ViewDesc viewdesc(&sdDesc);
			DescPtr descPtr = m_device->RtvHeap()->CreateView(m_texture.Get(), viewdesc);
			CPUHandles[(int)ViewType::RTV] = descPtr.cpuhandle;
			GPUHandles[(int)ViewType::RTV] = descPtr.gpuhandle;
		}
		{
			//srv view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = Format;
			srvDesc.ViewDimension = SrvDimension;
			srvDesc.Texture2D.MostDetailedMip = 0;      // 从最详细的Mip 0开始
			srvDesc.Texture2D.MipLevels = -1;           // 使用从 MostDetailedMip 开始的所有剩余Mip层级
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 不对LOD进行限制
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			DescriptorHeap::ViewDesc viewdesc(&srvDesc);
			DescPtr descPtr = m_device->SrvHeap()->CreateView(m_texture.Get(),viewdesc);
			CPUHandles[(int)ViewType::SRV] = descPtr.cpuhandle;
			GPUHandles[(int)ViewType::SRV] = descPtr.gpuhandle;
		}


		m_texture->SetName(L"RenderBuffer");
		StatusMap[GetTexture()] = {this,D3D12_RESOURCE_STATE_RENDER_TARGET};
	}

void ConstantBufferHeap::init(UINT size)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = (size + 255) & ~255;  // 对齐到256
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = GetDevice()->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,  // UPLOAD堆始终是 GENERIC_READ
    nullptr,
    IID_PPV_ARGS(&m_heap));

	m_heapSize = size;
	D3D12_RANGE range{ 0,0 };
	m_heap->Map(0,&range,&m_basePointer);
}


void ReadbackBufferHeap::init(UINT size)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = (size + 255) & ~255;  // 对齐到256
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = GetDevice()->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,  // UPLOAD堆始终是 GENERIC_READ
    nullptr,
    IID_PPV_ARGS(&m_heap));

	m_heapSize = size;
	D3D12_RANGE range{ 0,m_heapSize };
	m_heap->Map(0,&range,&m_basePointer);
}

UINT64 ReadbackBufferHeap::CopyResourceSync(ReadBackFunction* data,ID3D12GraphicsCommandList* cmdList)
{
	//todo: 需要判断之前的buffer有没有在用
	if (((m_offset + data->size + 255) & ~255) >= m_heapSize)
		m_offset = 0;
	
	if (data->type == ReadBackType::UAVBuffer)
	{
		auto value = GetResourceStatus(data->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
		if (!value)
		{
			D3D12_RESOURCE_BARRIER toCopySource = {};
			toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toCopySource.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toCopySource.Transition.pResource = data->resource;
			toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toCopySource.Transition.StateBefore = value.value().second;
			toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			cmdList->ResourceBarrier(1, &toCopySource);
		}
			cmdList->CopyBufferRegion(
				m_heap.Get(), m_offset,   
				data->resource, 0,       
				data->size);               

			D3D12_RESOURCE_BARRIER backToUav = {};
			backToUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			backToUav.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			backToUav.Transition.pResource = data->resource;
			backToUav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			backToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			backToUav.Transition.StateAfter = value.value().second;

			cmdList->ResourceBarrier(1, &backToUav);
			cmdList->Close();
			ID3D12CommandList* lists[] = { cmdList };
			Context::pContext->QueueDirect()->ExecuteCommandLists(1, lists);
			Context::pContext->WaitForGpu();
	}

	UINT64 temp = m_offset;
	m_offset += data->size;
	m_offset = (m_offset + 255) & ~255;
	return temp;
}


 void TextureBuffer:: CreateTexture()
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
        &m_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_texture));
		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = Format;
		srvDesc.ViewDimension = SrvDimension;
		srvDesc.Texture2D.MostDetailedMip = 0;      // 从最详细的Mip 0开始
		srvDesc.Texture2D.MipLevels = -1;           // 使用从 MostDetailedMip 开始的所有剩余Mip层级
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 不对LOD进行限制
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DescriptorHeap::ViewDesc viewdesc(&srvDesc);
		DescPtr descPtr = m_device->SrvHeap()->CreateView(m_texture.Get(),viewdesc);
		CPUHandles[(int)ViewType::SRV] = descPtr.cpuhandle;
		GPUHandles[(int)ViewType::SRV] = descPtr.gpuhandle;
	}

 	void TextureBuffer :: UploadTexture(const DirectX::Image* images,ID3D12GraphicsCommandList* cmdList)
	{
		

		D3D12_HEAP_PROPERTIES uploadHeapDesc{};
        uploadHeapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapDesc.CreationNodeMask = 1;
        uploadHeapDesc.VisibleNodeMask = 1;

	


				// 准备用于接收信息的结构
		UINT64 totalBytes = 0;                          // 复制整个纹理需要的总字节数
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprint(SubresourceCount);
		std::vector<UINT> numRowsArr(SubresourceCount);
		std::vector<UINT64> rowSizeArr(SubresourceCount);


		// 计算纹理的第一个子资源（subresource，索引0）的布局信息
		GetDevice()->GetCopyableFootprints(
			&m_desc,                    // 纹理描述
			0,                        // 子资源索引（如果是纹理数组/立方体贴图，需要指定）
			SubresourceCount,                        // 子资源数量（一次计算一个）
			0,                        // 基准偏移量（通常为0）
			footprint.data(),               // [输出] 布局信息
			numRowsArr.data(),                 // [输出] 行数
			rowSizeArr.data(),          // [输出] 每行字节数（这是重要结果）
			&totalBytes               // [输出] 总字节数（这就是你要的 memcpy 大小）
		);

			D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = totalBytes;  // 缓冲区大小
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1; //上传buffer只能是1
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;  // 缓冲区必须线性布局

		GetDevice()->CreateCommittedResource(&uploadHeapDesc, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,IID_PPV_ARGS(&tempUpload));

		D3D12_RANGE readRange{ 0, 0 };
        void* pData = nullptr;
        tempUpload->Map(0, &readRange, &pData);
		for (size_t i = 0; i < SubresourceCount; i++)
		{
			uint8_t* dst = (uint8_t*)pData + footprint[i].Offset;
			for (UINT row = 0; row < images[i].height; ++row) {
				memcpy((uint8_t*)dst,
					   images[i].pixels + row * images[i].rowPitch, // 源数据通常是紧密排列的
					   images[i].rowPitch);
				dst += footprint[i].Footprint.RowPitch;
			}

		D3D12_TEXTURE_COPY_LOCATION destLoc{};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = m_texture.Get();
		destLoc.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION srcLoc{};
		srcLoc.PlacedFootprint = footprint[i];
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.pResource = tempUpload.Get();

		cmdList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}
		
        tempUpload->Unmap(0, nullptr);


        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
		StatusMap[GetTexture()] = {this,D3D12_RESOURCE_STATE_COMMON};
	}

	TextureBuffer& TextureBuffer::GetDefaultWhiteTex()
	{
		static TextureBuffer defaultWhiteTex;
		static bool inited = false;
		if (!inited)
		{
			CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0, Context::pContext->FrameIndex(), CommandBufferPool::Type::DIRECT);
			defaultWhiteTex.Width = 1;
			defaultWhiteTex.Height = 1;
			defaultWhiteTex.CreateTexture();
			uint8_t data[4] = { 255,255,255,255 };
			defaultWhiteTex.UploadTexture(data,cmdbuffer.CmdList.Get());
			cmdbuffer.CmdList->Close();
			ID3D12CommandList* lists[] = { cmdbuffer.CmdList.Get()};
			Context::pContext->QueueDirect()->ExecuteCommandLists(1, lists);
			inited = true;
		}
		return defaultWhiteTex;
	}

	TextureBuffer& TextureBuffer::GetDefaultNormalTex()
	{
		static TextureBuffer defaultNormalTex;
		static bool inited = false;
		if (!inited)
		{
			CommandBuffer cmdbuffer = Context::pContext->GetCommandBufferPool()->AcquireCommandList(0, Context::pContext->FrameIndex(), CommandBufferPool::Type::DIRECT);
			defaultNormalTex.Width = 1;
			defaultNormalTex.Height = 1;
			defaultNormalTex.Srgb = false;
			defaultNormalTex.CreateTexture();
			uint8_t data[4] = { 128,128,255,255 };
			defaultNormalTex.UploadTexture(data,cmdbuffer.CmdList.Get());
			cmdbuffer.CmdList->Close();
			ID3D12CommandList* lists[] = { cmdbuffer.CmdList.Get()};
			Context::pContext->QueueDirect()->ExecuteCommandLists(1, lists);
			inited = true;
		}
		return defaultNormalTex;
	}

	void TextureBuffer :: UploadTexture(uint8_t* textureData,ID3D12GraphicsCommandList* cmdList)
	{
		

		D3D12_HEAP_PROPERTIES uploadHeapDesc{};
        uploadHeapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapDesc.CreationNodeMask = 1;
        uploadHeapDesc.VisibleNodeMask = 1;

	


				// 准备用于接收信息的结构
		UINT64 totalBytes = 0;                          // 复制整个纹理需要的总字节数
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;  // 包含行大小、宽度等信息
		UINT numRows = 0;                              // 行数
		UINT64 rowSizeInBytes = 0;                     // 每行的字节数

		// 计算纹理的第一个子资源（subresource，索引0）的布局信息
		GetDevice()->GetCopyableFootprints(
			&m_desc,                    // 纹理描述
			0,                        // 子资源索引（如果是纹理数组/立方体贴图，需要指定）
			MipLevels,                        // 子资源数量（一次计算一个）
			0,                        // 基准偏移量（通常为0）
			&footprint,               // [输出] 布局信息
			&numRows,                 // [输出] 行数
			&rowSizeInBytes,          // [输出] 每行字节数（这是重要结果）
			&totalBytes               // [输出] 总字节数（这就是你要的 memcpy 大小）
		);

			D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = totalBytes;  // 缓冲区大小
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1; //上传buffer只能是1
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;  // 缓冲区必须线性布局

		GetDevice()->CreateCommittedResource(&uploadHeapDesc, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,IID_PPV_ARGS(&tempUpload));

		D3D12_RANGE readRange{ 0, 0 };
        void* pData = nullptr;
        tempUpload->Map(0, &readRange, &pData);
		for (size_t i = 0; i < MipLevels; i++)
		{
			GetDevice()->GetCopyableFootprints(
			&m_desc,                    // 纹理描述
			i,                        // 子资源索引（如果是纹理数组/立方体贴图，需要指定）
			1,                        // 子资源数量（一次计算一个）
			0,                        // 基准偏移量（通常为0）
			&footprint,               // [输出] 布局信息
			&numRows,                 // [输出] 行数
			&rowSizeInBytes,          // [输出] 每行字节数（这是重要结果）
			&totalBytes               // [输出] 总字节数（这就是你要的 memcpy 大小）
			);

			for (UINT row = 0; row < numRows; ++row) {
			// 按行拷贝，每行拷贝 rowSizeInBytes 字节
			memcpy((uint8_t*)pData+footprint.Offset + row * footprint.Footprint.RowPitch, 
				   textureData + row * (m_desc.Width * GetPixelBytes(Format)), // 源数据通常是紧密排列的
				   rowSizeInBytes);
		}

		}
		
        tempUpload->Unmap(0, nullptr);

		D3D12_TEXTURE_COPY_LOCATION destLoc{};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = m_texture.Get();

		D3D12_TEXTURE_COPY_LOCATION srcLoc{};
		srcLoc.PlacedFootprint = footprint;
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.pResource = tempUpload.Get();

		cmdList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
		StatusMap[GetTexture()] = {this,D3D12_RESOURCE_STATE_COMMON};

	}
void InitEngineResource()
{
	TextureBuffer::GetDefaultWhiteTex();
	TextureBuffer::GetDefaultNormalTex();
}

void StructureBuffer::CreateBuffer()
{
	m_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	m_desc.Width = Width * Stride;
	m_desc.Height = 1;
	m_desc.DepthOrArraySize = 1;
	m_desc.MipLevels = 1;
	m_desc.Format = DXGI_FORMAT_UNKNOWN;
	m_desc.SampleDesc.Count = 1;
	m_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	m_desc.Flags = ReadWrite?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeap{};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeap.CreationNodeMask = 1;
	defaultHeap.VisibleNodeMask = 1;

	GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
    &m_desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
    IID_PPV_ARGS(&m_buffer));

	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = Width;
		uavDesc.Buffer.StructureByteStride = Stride;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		DescriptorHeap::ViewDesc viewdesc(&uavDesc);
		DescPtr descPtr = m_device->SrvHeap()->CreateUAView(m_buffer.Get(), viewdesc);
		CPUHandles[(int)ViewType::SRV] = descPtr.cpuhandle;
		GPUHandles[(int)ViewType::SRV] = descPtr.gpuhandle;
	}
	StatusMap[GetBuffer()] = {this,D3D12_RESOURCE_STATE_COMMON};
}
