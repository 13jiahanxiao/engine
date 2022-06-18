#include"PsoContainer.h"
#include"../Utility/ShaderCompile.h"
PsoContainer::PsoContainer(Device* device, ComPtr<ID3D12RootSignature> rootSign)
	: mDevice(device),mRootSign(rootSign)
{
	mShaders = std::make_unique<ShaderCompile>();
	BuildShadersAndInputLayout();
}
PsoContainer::~PsoContainer() 
{

}

D3D12_GRAPHICS_PIPELINE_STATE_DESC  PsoContainer::GetOpaquePsoDesc() 
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = mShaders->GetInputLayout();
	opaquePsoDesc.pRootSignature = mRootSign.Get();
	opaquePsoDesc.VS = mShaders->GetShaderBYTE("standardVS");
	opaquePsoDesc.PS = mShaders->GetShaderBYTE("opaquePS");
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	return opaquePsoDesc;
}

void PsoContainer::AddPsoContainer(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc,RenderLayer index) 
{
	ThrowIfFailed(mDevice->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPsos[index])));
}

void PsoContainer::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO rotate[] =
	{
		"ROT","1",NULL,NULL
	};

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG","1",NULL,NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG","1","ALPHA_TEST","1",NULL,NULL
	};

	mShaders->LoadShader("standardVS", L"Resources\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	//m_Shaders->LoadShader("NoTextureVS", L"Resources\\Shaders\\NoTexture.hlsl", nullptr, "VS", "vs_5_0");
	//m_Shaders->LoadShader("NoTexturePS", L"Resources\\Shaders\\NoTexture.hlsl", nullptr, "PS", "ps_5_0");
	mShaders->LoadShader("rotateVS", L"Resources\\Shaders\\Default.hlsl", rotate, "VS", "vs_5_1");
	mShaders->LoadShader("opaquePS", L"Resources\\Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders->LoadShader("alphaTestedPS", L"Resources\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders->SetInputLayout(
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		}
	);
}

D3D12_SHADER_BYTECODE PsoContainer::SetShader(std::string name)
{
	return mShaders->GetShaderBYTE(name);
}