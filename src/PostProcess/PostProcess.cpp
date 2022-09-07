#include"PostProcess.h"

PostProcess::PostProcess(Device* device,UINT descriptorSize) 
	:mDevice(device)
{

}

void PostProcess::InitBlurFilter(UINT width, UINT height, DXGI_FORMAT format)
{
	mClientHeight = height;
	mClientWidth = width;
	mBlurFilter = std::make_unique<BlurFilter>(mDevice->GetDevice(), mClientWidth, mClientHeight, format);

	mBlurFilter->BuildDescriptors(mDescriptorHeap);
}

void PostProcess::OnResize()
{
	if (mBlurFilter != nullptr)
	{
		mBlurFilter->OnResize(mClientWidth, mClientHeight);
	}
}

void PostProcess::Execute(ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSig,
	ID3D12PipelineState* horzBlurPSO,
	ID3D12PipelineState* vertBlurPSO,
	ID3D12Resource* input,
	int blurCount)
{
	mBlurFilter->Execute(cmdList, rootSig, horzBlurPSO, vertBlurPSO, input, blurCount);
}

ID3D12Resource* PostProcess::BlurFilterOutput()
{
	return mBlurFilter->Output();
}