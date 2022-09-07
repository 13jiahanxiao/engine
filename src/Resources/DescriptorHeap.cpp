#include"DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(Device* device,D3D12_DESCRIPTOR_HEAP_TYPE Type,uint64 numDescriptors,bool bShaderVisible)
	: Resource(device),
	allocatePool(numDescriptors),
	m_numDescriptors(numDescriptors) ,
	m_heapUseIndex(-1)
{
	m_Desc.NumDescriptors = numDescriptors;
	m_Desc.Type = Type;
	m_Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_Desc.NodeMask = 0;
	{
		for (size_t i = 0; i < numDescriptors; ++i) {
			allocatePool[i] = i;
		}
	}
	ThrowIfFailed(device->GetDevice()->CreateDescriptorHeap(&m_Desc,IID_PPV_ARGS(&m_DescHeap)));
	m_hCPUHeapStart = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
	m_hGPUHeapStart = m_DescHeap->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = device->GetDevice()->GetDescriptorHandleIncrementSize(m_Desc.Type);
}
DescriptorHeap::~DescriptorHeap() 
{

}

uint64 DescriptorHeap::CreateUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& pDesc)
{
	m_heapUseIndex++;
	m_Device->GetDevice()->CreateUnorderedAccessView(resource, nullptr, &pDesc, HCPU(m_heapUseIndex));
	return m_heapUseIndex;
}
uint64 DescriptorHeap::CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& pDesc)
{
	m_heapUseIndex++;
	m_Device->GetDevice()->CreateShaderResourceView(resource, &pDesc, HCPU(m_heapUseIndex));
	return m_heapUseIndex;
}