#include"PsoContainer.h"

PsoContainer::PsoContainer(Device* device) :
	m_Device(device)
{
}

PsoContainer::~PsoContainer()
{
}

void CreateOpaquePipelineState() 
{
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC  opaquePsoDesc;
	//ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	//opaquePsoDesc.pRootSignature = mRootSignature.Get();
	//opaquePsoDesc.VS =
	//{
	//	reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
	//	mShaders["standardVS"]->GetBufferSize()
	//};
	//opaquePsoDesc.PS =
	//{
	//	reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
	//	mShaders["opaquePS"]->GetBufferSize()
	//};
	//opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	//opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//opaquePsoDesc.SampleMask = UINT_MAX;
	//opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//opaquePsoDesc.NumRenderTargets = 1;
	//opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	//opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	//opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	//ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));
}