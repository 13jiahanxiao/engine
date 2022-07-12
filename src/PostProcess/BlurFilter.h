//***************************************************************************************
// BlurFilter.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs a blur operation on the topmost mip level of an input texture.
//***************************************************************************************

#pragma once

#include"../Resources/DescriptorHeap.h"
#include "../Utility/d3dUtil.h"

class BlurFilter
{
public:
	///<summary>
	/// The width and height should match the dimensions of the input texture to blur.
	/// Recreate when the screen is resized. 
	///</summary>
	BlurFilter(ID3D12Device* device,UINT width, UINT height,DXGI_FORMAT format);

	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

	ID3D12Resource* Output();

	void BuildDescriptors(DescriptorHeap* heap, UINT mNowHeapSize);

	void OnResize(UINT newWidth, UINT newHeight);

	///<summary>
	/// Blurs the input texture blurCount times.
	///</summary>
	void Execute(
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* horzBlurPSO,
		ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* input,
		int blurCount);

private:
	std::vector<float> CalcGaussWeights(float sigma);

	void BuildDescriptors();
	void BuildResources();

private:

	const int MaxBlurRadius = 5;

	ID3D12Device* mDevice = nullptr;
	DescriptorHeap* mDescHeap = nullptr;


	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	UINT mBlur0CpuSrv;
	UINT mBlur0CpuUav;

	UINT mBlur1CpuSrv;
	UINT mBlur1CpuUav;

	UINT mBlur0GpuSrv;
	UINT mBlur0GpuUav;

	UINT mBlur1GpuSrv;
	UINT mBlur1GpuUav;

	// 分开处理纵向和横向的模糊
	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1 = nullptr;
};
