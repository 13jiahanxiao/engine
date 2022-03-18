#pragma once
#include"../DXRuntime/Device.h"
#include <d3d12.h>
#include <unordered_map>
#include <wrl.h>
class PsoContainer
{
public:
	PsoContainer(Device* device);
	~PsoContainer();
	//Ä¬ÈÏÎªopaque
	void CreateOpaquePipelineState();
private:
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PSOs;
	Device* m_Device;
};

