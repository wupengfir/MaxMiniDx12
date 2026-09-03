#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include "Resource.h"
#include "Constants.h"
#include "Material.h"
using Microsoft::WRL::ComPtr;



class DSVDescriptorHeap;
class RTVDescriptorHeap;
class CommandBufferPool;
class MyMesh;

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

class GraphicsDevice : public Device
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
    uint32_t m_frameIndex = 0;
	HANDLE   m_fenceEvent = nullptr;
	DSVDescriptorHeap* m_DsvHeap;
	RTVDescriptorHeap* m_RtvHeap;
	CommandBufferPool* m_commandBufferPool;
	ComPtr<ID3D12Resource> m_backBuffers[FRAME_BACKBUFFER_COUNT];
	
	bool useDebug;
public :
	UINT ScreenWidth;
	UINT ScreenHeight;
	static inline GraphicsDevice* pDefaultDevice = nullptr;
	void SyncGPU(UINT64 signal);
	void BeginFrame();  
    void EndFrame();                          // 关闭+提交+Present+Signal
    void WaitForGpu();                        // 等 GPU 全部完成（退出前用）
	ID3D12Resource* GetBackBuffer(uint32_t i) const { return m_backBuffers[i].Get(); }
	ID3D12CommandQueue* QueueDirect() { return m_queueDirect.Get(); }
	ID3D12CommandQueue* QueueCompute() { return m_queueCompute.Get(); }
	ID3D12CommandQueue* QueueCopy() { return m_queueCopy.Get(); }
	DSVDescriptorHeap* DsvHeap() { return m_DsvHeap; }
	RTVDescriptorHeap* RtvHeap() { return m_RtvHeap; }
	CommandBufferPool* GetCommandBufferPool(){ return m_commandBufferPool; }
	bool Init(HWND hwnd, uint32_t width, uint32_t height);
	void DrawMesh(ID3D12GraphicsCommandList* cmdList,MyMesh* mesh,Material* mat);


	
	uint32_t FrameIndex() { return m_frameIndex; }

	GraphicsDevice(bool useDebugLayer = true);
	~GraphicsDevice()
	{
		delete m_DsvHeap;
		delete m_RtvHeap;
		delete m_commandBufferPool;
	}
};

