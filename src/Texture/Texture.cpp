#include"Texture.h"
#include"../Resources/DescriptorHeap.h"
#include"../Tools/tinyxml2.h"
using namespace  tinyxml2;

TextureManager::TextureManager()
{

}
TextureManager::~TextureManager()
{

}

std::wstring string2wstring(std::string str)
{
	std::wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

void TextureManager::LoadTextureFormXML(std::string name, std::string fileName, int index)
{
	auto Tex = std::make_unique<Texture>();
	Tex->Name = name;
	Tex->Filename = string2wstring(fileName);
	Tex->heapIndex = index;
	m_Textures[Tex->Name] = std::move(Tex);
}

void TextureManager::LoadTextureXML()
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile("Resources\\XML\\textureConfig.xml");

	XMLElement* textureHanlde = doc.FirstChildElement("texture");
	if (textureHanlde)
	{
		XMLElement* item = textureHanlde->FirstChildElement("Item");
		const char* sztext = NULL;
		while (item)
		{
			sztext = item->Attribute("name");
			std::string name(sztext);
			sztext = item->Attribute("address");
			std::string address(sztext);
			sztext = item->Attribute("index");
			int index = atoi(sztext);
			LoadTextureFormXML(name, address, index);
			item = item->NextSiblingElement("Item");
		}
	}
}

void TextureManager::LoadTexture(Device* device, ID3D12GraphicsCommandList* cmdList)
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
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		heap->CreateSRV(tex.Get(), srvDesc, v->heapIndex);
	}
	printf("加载图片完成\n");
}