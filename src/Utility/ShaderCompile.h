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

	void SetInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout) 
	{
		m_InputLayout = inputLayout;
	}
	D3D12_INPUT_LAYOUT_DESC GetInputLayout()
	{
		return { m_InputLayout.data(),(UINT)m_InputLayout.size() };
	}
private:
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_Shaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
};