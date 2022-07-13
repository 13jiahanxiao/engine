#pragma once
#include"../DXRuntime/Device.h"
#include"../Utility/d3dUtil.h"
class RootSignature
{
public:
	RootSignature(Device* device);
	~RootSignature() = default;

	void Init(UINT textureNum);

	void InitRootSignature(UINT textureNum);
	void InitPostProcess();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSign() {return mRootSignature;}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetPostProcessRootSign() { return mPostProcessRootSignature; }

private:
	Device* mDevice;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mPostProcessRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
};
