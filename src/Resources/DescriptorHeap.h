#pragma once
#include "Resource.h"
#include <mutex>
class DescriptorHeap final :public Resource 
{
public:
	CD3DX12_GPU_DESCRIPTOR_HANDLE HGPU(uint64 index) const
	{
		if (index >= m_Desc.NumDescriptors) index = m_Desc.NumDescriptors - 1;
		D3D12_GPU_DESCRIPTOR_HANDLE h = { m_hGPUHeapStart.ptr + index * HandleIncrementSize };
		CD3DX12_GPU_DESCRIPTOR_HANDLE h1(h);
		return h1;	
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE NowHGPU() const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE h = { m_hGPUHeapStart.ptr + m_heapUseIndex * HandleIncrementSize };
		CD3DX12_GPU_DESCRIPTOR_HANDLE h1(h);
		return h1;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE HCPU(uint64 index) const
	{
		if (index >= m_Desc.NumDescriptors) index = m_Desc.NumDescriptors - 1;
		D3D12_CPU_DESCRIPTOR_HANDLE h = { m_hCPUHeapStart.ptr + index * HandleIncrementSize };
		CD3DX12_CPU_DESCRIPTOR_HANDLE h1(h);
		return h1;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE NowHCPU() const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h = { m_hCPUHeapStart.ptr + m_heapUseIndex * HandleIncrementSize };
		CD3DX12_CPU_DESCRIPTOR_HANDLE h1(h);
		return h1;
	}
	DescriptorHeap(Device* pDevice,D3D12_DESCRIPTOR_HEAP_TYPE Type,uint64 numDescriptors,bool bShaderVisible);
	~DescriptorHeap();
	uint64 CreateUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& pDesc);
	uint64 CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& pDesc);
	uint64 GetNum() const { return m_numDescriptors; }
	ID3D12DescriptorHeap* GetHeap() const { return m_DescHeap.Get(); }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart()
	{
		return m_hCPUHeapStart;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart()
	{
		return m_hGPUHeapStart;
	}
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_Desc;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCPUHeapStart;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGPUHeapStart;
	uint HandleIncrementSize;
	uint64 m_numDescriptors;
	std::vector<uint> allocatePool;
	std::mutex heapMtx;
	//µ±Ç°µÄindex
	uint64 m_heapUseIndex;
};