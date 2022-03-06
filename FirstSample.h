#pragma once
#include"DXSample.h"
#include"Device.h"
#include"ReflactableStruct.h"
#include"Common/d3dx12.h"
#include"Mesh.h"
using namespace DirectX;
using Microsoft::WRL::ComPtr;

class FirstSample :public DXSample 
{
public :
	FirstSample(uint width, uint height, std::wstring name);
	~FirstSample();
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;
private:
	static const uint FrameCount = 2;

	struct Vertex :public utility::TypeStruct 
	{
		utility::Var<XMFLOAT3> position = "POSITION";
		utility::Var<XMFLOAT4> color = "COLOR";
	};

	std::unique_ptr<Device> device;

	CD3DX12_VIEWPORT m_viewport;

	CD3DX12_RECT m_scissorRect;

	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	uint m_rtvDescriptorSize;

	std::unique_ptr<Mesh> triangleMesh;

	uint m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	uint64 m_fenceValue;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};