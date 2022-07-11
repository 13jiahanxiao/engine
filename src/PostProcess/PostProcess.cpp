#include"PostProcess.h"

PostProcess::PostProcess(Device* device,UINT descriptorSize) 
	:mDevice(device),descriptorSize(descriptorSize)
{

}

void PostProcess::InitBlurFilter(UINT width, UINT height, DXGI_FORMAT format)
{
	mClientHeight = height;
	mClientWidth = width;
	mBlurFilter = std::make_unique<BlurFilter>(mDevice->GetDevice(), mClientWidth, mClientHeight, format);

	mBlurFilter->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mNowHeapSize, mPostProcessHeapSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), mNowHeapSize, mPostProcessHeapSize),
		mPostProcessHeapSize);
}

void PostProcess::OnResize()
{
	if (mBlurFilter != nullptr)
	{
		mBlurFilter->OnResize(mClientWidth, mClientHeight);
	}
}