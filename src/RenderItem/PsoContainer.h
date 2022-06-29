#pragma once
#include"RenderItem.h"

class ShaderCompile;
class PsoContainer
{
public:
	PsoContainer(Device* device, ComPtr<ID3D12RootSignature> rootSign);
	~PsoContainer();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  GetOpaquePsoDesc();
	D3D12_DEPTH_STENCIL_DESC  GetStencilDefault();
	void AddPsoContainer(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, RenderLayer index);
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> GetPsoMap() { return mPsos; }
	ID3D12PipelineState* GetPsoByRenderLayer(RenderLayer index)
	{
		if (mPsos.find(index) != mPsos.end()) 
		{
			return mPsos[index].Get();
		}
		return NULL;
	}
	void BuildShadersAndInputLayout();
	D3D12_SHADER_BYTECODE SetShader(std::string name);
	D3D12_INPUT_LAYOUT_DESC GetVerInputLayout();
	D3D12_INPUT_LAYOUT_DESC GetGeoInputLayout();
private:
	Device* mDevice;
	ComPtr<ID3D12RootSignature> mRootSign;
	//Œﬁ–Ú”≥…‰π‹¿Ì
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> mPsos;

	std::unique_ptr<ShaderCompile> mShaders;

	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
