#pragma once
#include "../DXRuntime/Device.h"
using Microsoft::WRL::ComPtr;

class Resource 
{
public:
	Device* GetDevice()  { return m_Device; }
	Resource(Device* device): m_Device(device) {}
	Resource(Resource&&) = default;
	Resource(Resource const&) = delete;
	virtual ~Resource() = default;
	virtual ID3D12Resource* GetResource() const { return nullptr; }
	virtual D3D12_RESOURCE_STATES GetInitState() const { return D3D12_RESOURCE_STATE_COMMON; }
protected:
	Device* m_Device;
};
