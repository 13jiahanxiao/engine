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

D3D12_DEPTH_STENCIL_DESC   PsoContainer::GetStencilDefault()
{
	D3D12_DEPTH_STENCIL_DESC mirrorDSS;
	//开启深度测试
	mirrorDSS.DepthEnable = true;
	//深度测试但是禁止写入
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//深度小则接受
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	//模板测试开启
	mirrorDSS.StencilEnable = true;
	//0xff不会屏蔽任何一位模板值
	mirrorDSS.StencilReadMask = 0xff;
	//0xff不会屏蔽任何一位模板值 防止前4位被改写 0x0f
	mirrorDSS.StencilWriteMask = 0xff;

	//未通过模板测试，该如何操作，保持不变
	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	//通过模板未通过深度
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	//都通过，替代
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	//比较函数
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// 背面设置
	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	return mirrorDSS;
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