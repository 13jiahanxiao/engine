#include"Texture.h"
#include"../Resources/DescriptorHeap.h"
#include"../Tools/tinyxml2.h"
#include"../Utility/d3dUtil.h"
#include"nlohmann/json.hpp"
using json = nlohmann::json;

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{

}

void TextureManager::Init() 
{
	LoadTextureJson();
}

void TextureManager::LoadTextureFormJson(std::string name, std::string fileName, int index,int dimension)
{
	auto Tex = std::make_unique<Texture>();
	Tex->Name = name;
	Tex->Filename = d3dUtil::String2Wstring(fileName);
	Tex->heapIndex = index;
	Tex->Dimension = (D3D12_SRV_DIMENSION)dimension;
	m_Textures[Tex->Name] = std::move(Tex);
}

void TextureManager::LoadTextureJson()
{
	std::ifstream f("Resources\\Json\\TextureConfig.json");
	json data = json::parse(f);
	std::string address = data.at("Address");
	for (int i = 0; i < data["TextureConfig"].size(); i++)
	{
		std::string name = data["TextureConfig"][i].at("name");
		std::string fileName = address;
		fileName += data["TextureConfig"][i].at("fileName");
		int index = data["TextureConfig"][i].at("index");
		int dimension = data["TextureConfig"][i].at("dimension");
		LoadTextureFormJson(name, fileName, index, dimension);
	}
}

void TextureManager::CreateDDSTexture(Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& [k, v] : m_Textures) 
	{
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device->GetDevice(), cmdList,
			v->Filename.c_str(),
			v->Resource, v->UploadHeap));
	}
	printf("创建图片资源完成\n");
}

void TextureManager::BuildTextureHeap(DescriptorHeap* heap)
{
	for (auto& [k, v] : m_Textures)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		auto tex = m_Textures[v->Name]->Resource;

		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = tex->GetDesc().Format;
		srvDesc.ViewDimension = m_Textures[v->Name]->Dimension;
		if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE) 
		{
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = tex->GetDesc().MipLevels;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		else 
		{
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}


		heap->CreateSRV(tex.Get(), srvDesc, v->heapIndex);
	}
	printf("加载图片完成\n");
}