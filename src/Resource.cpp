#include "Resource.h"
#include <iostream>
#include <comdef.h>
#include "context.h"
GraphicsDevice* Resource::m_device = nullptr;

ID3D12Device* Resource::GetDevice() const { return m_device->DxDevice(); }

CommandBuffer::~CommandBuffer()
{
	m_pool->FreeCommandBuffer(this->CmdList);
}

//void CommandBuffer::Release()
//{
//	m_pool->FreeCommandBuffer(this->CmdList);
//}

void RTVDescriptorHeap::CreateView(ID3D12Resource* resource, ViewDesc desc)
{
	GetDevice()->CreateRenderTargetView(resource, desc.RtView, Handle);
	m_DescResourceMap[resource] = DescPtr{ this,Handle };
	Handle.ptr += Size;
}

void DSVDescriptorHeap::CreateView(ID3D12Resource* resource,ViewDesc desc)
{
	m_device->DxDevice()->CreateDepthStencilView(resource, desc.DsView, Handle);
	m_DescResourceMap[resource] = DescPtr{ this,Handle };
	Handle.ptr += Size;
}

void SRVDescriptorHeap::CreateView(ID3D12Resource* resource,ViewDesc desc) 
{
	m_device->DxDevice()->CreateShaderResourceView(resource, desc.SrView, Handle);
	m_DescResourceMap[resource] = DescPtr{ this,Handle };
	Handle.ptr += Size;
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

		D3D12_DEPTH_STENCIL_VIEW_DESC sdDesc = {};
		sdDesc.Format = DXGI_FORMAT_D32_FLOAT;
		sdDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		sdDesc.Texture2D.MipSlice = 0;
		DescriptorHeap::ViewDesc viewdesc(&sdDesc);
		m_device->DsvHeap()->CreateView(m_texture.Get(), viewdesc);

		m_texture->SetName(L"DepthBuffer");
	}