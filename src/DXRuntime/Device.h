#pragma once
#include"../Utility/d3dUtil.h"

class Device 
{
public:
	Device();
	~Device();
	IDXGIAdapter1* GetAdapter() const { return m_Adapter.Get(); }
	ID3D12Device5* GetDevice() const { return m_Device.Get(); }
	IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }

	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_Adapter;
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
};