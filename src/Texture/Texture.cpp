#include"Texture.h"
#include"../Resources/DescriptorHeap.h"
#include"../Tools/tinyxml2.h"
#include"../Utility/d3dUtil.h"
#include"nlohmann/json.hpp"
#include"../../ThirdParty/stb/stb_image.h"
#include"../Utility/d3dx12.h"
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
	Tex->Filename = fileName;
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

void TextureManager::CreateTexture(Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& [k, v] : m_Textures)
	{
		CreateTexture(device->GetDevice(), cmdList,v->Filename,
			v->Resource, v->UploadHeap);
	}
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

void TextureManager::CreateTexture( ID3D12Device* device,
	 ID3D12GraphicsCommandList* cmdList,
	 std::string fileName,
	_Out_ ComPtr<ID3D12Resource>& texture,
	_Out_ ComPtr<ID3D12Resource>& textureUploadHeap) 
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	const UINT TexturePixelSize = 4;//由于是 rgb_alpha 所以是4通道

	D3D12_RESOURCE_DESC textureDesc = {};
	//暂定为1
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = texWidth;
	textureDesc.Height = texHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	//创建szFileName：实际用的纹理资源   
	device->CreateCommittedResource(
		rvalue_to_lvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(texture.GetAddressOf()));

	const UINT num2DSubresources = textureDesc.DepthOrArraySize * textureDesc.MipLevels;
	//尺寸：
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, num2DSubresources);

	// Create the GPU upload buffer.
	ThrowIfFailed(device->CreateCommittedResource(
		rvalue_to_lvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		rvalue_to_lvalue(CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(textureUploadHeap.GetAddressOf())));


	//将数据复制给中间资源，然后从中间资源复制给实际的资源
	cmdList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = pixels;
	textureData.RowPitch = texWidth * TexturePixelSize;
	textureData.SlicePitch = textureData.RowPitch * texHeight;
	UpdateSubresources(cmdList, texture.Get(), textureUploadHeap.Get(), 0, 0, num2DSubresources, &textureData);

	cmdList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)));
}