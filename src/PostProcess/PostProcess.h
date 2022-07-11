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

	UINT GetHeapSize() { return mPostProcessHeapSize; };

	void EffectBlurFilter() { mPostProcessHeapSize += 4; }

	void SetDescriptorHeapAndOffset(DescriptorHeap* heap, UINT heapSize) {
		mDescriptorHeap = heap;
		mNowHeapSize = heapSize;
	}

	void InitBlurFilter(UINT width, UINT height, DXGI_FORMAT format);

	void OnResize();
private:
	Device* mDevice;
	DescriptorHeap* mDescriptorHeap;
	UINT descriptorSize = 0;
	
	UINT mPostProcessHeapSize = 0;
	UINT mNowHeapSize = 0;

	std::unique_ptr<BlurFilter> mBlurFilter;

	UINT mClientWidth = 0;
	UINT mClientHeight = 0;
};
