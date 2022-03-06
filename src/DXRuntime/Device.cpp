#include"Device.h"
#include"../Utility/Metalib.h"
#include <cstdint>
Device::Device() 
{
	using Microsoft::WRL::ComPtr;
	uint32_t dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)));
	uint adapterIndex = 0;	  // we'll start looking for directx 12  compatible graphics devices starting at index 0
	bool adapterFound = false;// set this to true when a good one was found
	while (m_Factory->EnumAdapters1(adapterIndex, &m_Adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		m_Adapter->GetDesc1(&desc);
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
			HRESULT hr = D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_1,
				IID_PPV_ARGS(&m_Device));
			if (SUCCEEDED(hr)) {
				adapterFound = true;
				break;
			}
		}
		m_Adapter = nullptr;
		adapterIndex++;
	}
}

Device::~Device() {}