#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <string>
#include <unordered_map>
class ShaderCompile 
{
public:
	ShaderCompile();
	~ShaderCompile();

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	void LoadShader(const std::string name,
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	D3D12_SHADER_BYTECODE GetShaderBYTE(std::string name);

	void SetVerInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout) 
	{
		mVerInputLayout = inputLayout;
	}
	void SetGeoInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout)
	{
		mGeoInputLayout = inputLayout;
	}
	D3D12_INPUT_LAYOUT_DESC GetVerInputLayout()
	{
		return { mVerInputLayout.data(),(UINT)mVerInputLayout.size() };
	}
	D3D12_INPUT_LAYOUT_DESC GetGeoInputLayout()
	{
		return { mGeoInputLayout.data(),(UINT)mGeoInputLayout.size() };
	}
private:
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mVerInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mGeoInputLayout;
};