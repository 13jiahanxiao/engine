#include"ShaderCompile.h"
#include "d3dUtil.h"
#include<d3d12.h>
#include <d3dcompiler.h>

ShaderCompile::ShaderCompile()
{

}
ShaderCompile::~ShaderCompile()
{

}

Microsoft::WRL::ComPtr<ID3DBlob> ShaderCompile::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

void ShaderCompile::LoadShader(const std::string name,
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	m_Shaders[name] = CompileShader(filename, defines, entrypoint, target);
}


D3D12_SHADER_BYTECODE ShaderCompile::GetShaderBYTE(std::string name)
{
	return
	{
		reinterpret_cast<BYTE*>(m_Shaders[name]->GetBufferPointer()),
		m_Shaders[name]->GetBufferSize()
	};
}
