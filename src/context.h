#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include "Resource.h"
#include "Constants.h"
#include "Material.h"
using Microsoft::WRL::ComPtr;

inline static UINT GlobalID = 0;
inline static std::unordered_map<std::string, UINT> GlobalIDMap;
inline static UINT PropertyToID(std::string name)
{
	auto it = GlobalIDMap.find(name);
	if (it != GlobalIDMap.end())
	{
		return it->second;
	}
	return GlobalID++;
}



class ConstantBufferHeap;
class DSVDescriptorHeap;
class RTVDescriptorHeap;
class CommandBufferPool;
class Mesh;
class Camera;
class Device
{
protected:
	ComPtr<IDXGIFactory4> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;
	ComPtr<ID3D12Device> m_device;
public :
	
	IDXGIFactory4* Factory() { return m_factory.Get(); }
	IDXGIAdapter1* Adapter() { return m_adapter.Get(); }
	ID3D12Device* DxDevice() { return m_device.Get(); }
};

class Context : public Device
{
private :

	ComPtr<ID3D12Debug> m_debug;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12CommandQueue>   m_queueDirect;
	ComPtr<ID3D12CommandQueue>   m_queueCompute;
	ComPtr<ID3D12CommandQueue>   m_queueCopy;
	
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_nextFenceValue = 1;                  // 全局递增的 fence 值
    uint64_t m_frameFenceValues[FRAME_BACKBUFFER_COUNT] = {}; // 每个 buffer 最后一次的 fence 值
	DescPtr m_swapChainViews[FRAME_BACKBUFFER_COUNT] = {};
    uint32_t m_frameIndex = 0;
	HANDLE   m_fenceEvent = nullptr;
	ConstantBufferHeap* m_CBufferHeap;
	ReadbackBufferHeap* m_ReadbackBufferHeap;
	DSVDescriptorHeap* m_DsvHeap;
	RTVDescriptorHeap* m_RtvHeap;
	SRVDescriptorHeap* m_SrvHeap;
	CommandBufferPool* m_commandBufferPool;
	ComPtr<ID3D12Resource> m_backBuffers[FRAME_BACKBUFFER_COUNT];
	


	bool useDebug;
public :
	UINT ScreenWidth;
	UINT ScreenHeight;
	DepthTextureBuffer depthbuffer;
    RenderTextureBuffer colorbuffer;
	inline static RenderSetting GlobalSetting{};
	static inline Context* pContext = nullptr;
	void SyncGPU(UINT64 signal);
	void BeginFrame();  
    void EndFrame();                          // 关闭+提交+Present+Signal
    void WaitForGpu();                        // 等 GPU 全部完成（退出前用）
	void CreateFrameBuffers();
	void ResizeFrameBuffers();
	void Update();
	ID3D12Resource* GetBackBuffer(uint32_t i) const { return m_backBuffers[i].Get(); }
	DescPtr GetSwapChainView() const { return m_swapChainViews[m_frameIndex]; }
	ID3D12CommandQueue* QueueDirect() { return m_queueDirect.Get(); }
	ID3D12CommandQueue* QueueCompute() { return m_queueCompute.Get(); }
	ID3D12CommandQueue* QueueCopy() { return m_queueCopy.Get(); }
	DSVDescriptorHeap* DsvHeap() { return m_DsvHeap; }
	RTVDescriptorHeap* RtvHeap() { return m_RtvHeap; }
	SRVDescriptorHeap* SrvHeap() { return m_SrvHeap; }
	ConstantBufferHeap* CBufferHeap() { return m_CBufferHeap; }
	ReadbackBufferHeap* ReadBufferHeap() { return m_ReadbackBufferHeap; }
	CommandBufferPool* GetCommandBufferPool(){ return m_commandBufferPool; }
	bool Init(HWND hwnd, uint32_t width, uint32_t height);
	void DrawMesh(ID3D12GraphicsCommandList* cmdList,Mesh* mesh,int submeshIndex,Material* mat,const XMMATRIX* matrix ,Camera* camera,DXGI_FORMAT formats[] ,const XMMATRIX* matrixInv = nullptr);
	void Dispatch(ID3D12GraphicsCommandList* cmdList, Material* mat, UINT x, UINT y, UINT z);
	void ShutDown();
	
	uint32_t FrameIndex() { return m_frameIndex; }

	Context(bool useDebugLayer = true);
	~Context()
	{
		delete m_DsvHeap;
		delete m_RtvHeap;
		delete m_SrvHeap;
		delete m_commandBufferPool;
		delete m_CBufferHeap;
		delete m_ReadbackBufferHeap;
	}
};

