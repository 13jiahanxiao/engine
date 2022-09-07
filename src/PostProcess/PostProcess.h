#pragma once

#include "../Utility/d3dUtil.h"
#include"../DXRuntime/Device.h"
#include"../Resources/DescriptorHeap.h"
#include"BlurFilter.h"
class PostProcess
{
public:
	PostProcess(Device* device, UINT descriptorSize);
	~PostProcess()=default;


	void SetDescriptorHeapAndOffset(DescriptorHeap* heap) {
		mDescriptorHeap = heap;
	}

	void InitBlurFilter(UINT width, UINT height, DXGI_FORMAT format);

	void OnResize();

	void Execute(ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* horzBlurPSO,
		ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* input,
		int blurCount);

	ID3D12Resource* BlurFilterOutput();
private:
	Device* mDevice;
	DescriptorHeap* mDescriptorHeap;
	
	std::unique_ptr<BlurFilter> mBlurFilter;

	UINT mClientWidth = 0;
	UINT mClientHeight = 0;
};
