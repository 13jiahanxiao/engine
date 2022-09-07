#include"LoadAsset.h"
#include"LoadObject.h"
#include"../Tools/tinyxml2.h"
#include"../Utility/d3dUtil.h"
#include"nlohmann/json.hpp"
#include"../../ThirdParty/stb/stb_image.h"
#include"../Utility/d3dx12.h"
#include"DDSTextureLoader.h"
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
using json = nlohmann::json;

using namespace REngine;
LoadAsset::LoadAsset()
{
	m_meshData.clear();
	LoadObjectFromJson();
}

LoadAsset::~LoadAsset()
{
}

void LoadAsset::LoadObjectFromJson() 
{
	std::ifstream f("Resources/Json/LoadObejct.json");
	json data = json::parse(f);
	std::string address = data.at("Address");
	for (int i = 0; i < data["ObjectConfig"].size(); i++)
	{
		std::string name = data["ObjectConfig"][i].at("name");
		std::string fileName = address;
		fileName += data["ObjectConfig"][i].at("fileName");
		ParseObjectData(name, fileName);
	}
}

void LoadAsset::ParseObjectData(std::string geoName, std::string fileName)
{
	MeshData pMeshData;
	pMeshData.m_name = geoName;

	Assimp::Importer aiImporter;
	const aiScene* pModel = aiImporter.ReadFile(fileName, aiProcess_ConvertToLeftHanded);
	if (nullptr == pModel)
	{
		return;
	}
	int pStartVertex = 0;
	int pIndex = 0;
	if (pModel->HasMeshes())
	{
		for (int num = 0; num < pModel->mNumMeshes; num++)
		{
			SubMeshData pSub;
			pSub.m_startIndex = pIndex;
			pSub.m_startVertex = pStartVertex;
			aiMesh* pMesh = pModel->mMeshes[num];
			if (pMesh->HasFaces())
			{
				for (int i = 0; i < pMesh->mNumVertices; i++)
				{
					Vertex ver;
					ver.Pos.x = pMesh->mVertices[i].x;
					ver.Pos.y = pMesh->mVertices[i].y;
					ver.Pos.z = pMesh->mVertices[i].z;
					ver.TexC.x = pMesh->mTextureCoords[0][i].x;
					ver.TexC.y = pMesh->mTextureCoords[0][i].y;
					ver.Normal.x = pMesh->mNormals[i].x;
					ver.Normal.y = pMesh->mNormals[i].y;
					ver.Normal.z = pMesh->mNormals[i].z;
					pMeshData.m_vertices.push_back(ver);
				}
				pStartVertex += pMesh->mNumVertices;
				for (int i = 0; i < pMesh->mNumFaces; i++)
				{
					aiFace face = pMesh->mFaces[i];
					for (int j = 0; j < face.mNumIndices; j++)
						pMeshData.m_indices.push_back(face.mIndices[j]);
				}
				pSub.m_indexSize = pMesh->mNumFaces * 3;
				pIndex += pSub.m_indexSize;
			}
			pSub.m_name = pMesh->mName.C_Str();
			if (pModel->HasMaterials())
			{
				aiMaterial* pMaterial = pModel->mMaterials[pMesh->mMaterialIndex];
				aiString aistr;
				std::string pTexName = "Resources/Models/";
				pTexName +=geoName;
				pTexName += "/";
				pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);
				std::string pTexPath = aistr.C_Str();
				if (pTexPath.size() > 0)
				{
					pTexName += pTexPath;
					SaveTextureData(pTexName);
					pSub.material = pTexName;
				}
			}
			pSub.m_name = pMesh->mName.C_Str();
			pMeshData.m_submesh.push_back(pSub);
		}
	}

	m_meshData.push_back(pMeshData);
}

void LoadAsset::SaveMaterialData(std::string name, int index)
{
	if (m_materials.find(name) != m_materials.end())
		return;
	LoadMaterialData mat;
	mat.DiffuseMapIndex = index;
	m_materials[name] = mat;
}

void LoadAsset::SaveTextureData(std::string fileName, int dimension)
{
	if (m_textures.find(fileName) != m_textures.end())
		return;
	auto Tex = std::make_unique<Texture>();
	Tex->Name = fileName;
	Tex->Filename = fileName;
	Tex->Dimension = (D3D12_SRV_DIMENSION)dimension;
	m_textures[Tex->Name] = std::move(Tex);
}

void LoadAsset::LoadTextureFromJson()
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
		SaveTextureData( fileName, dimension);
	}
}

void LoadAsset::CreateDDSTexture(Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& [k, v] : m_textures)
	{
		std::wstring temp = d3dUtil::String2Wstring(v->Filename);
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device->GetDevice(), cmdList,
			temp.c_str(),
			v->Resource, v->UploadHeap));
	}
}

void LoadAsset::CreateTexture(Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& [k, v] : m_textures)
	{
		CreateTexture(device->GetDevice(), cmdList, v->Filename,
			v->Resource, v->UploadHeap);
	}
}

void LoadAsset::BuildTextureHeap(DescriptorHeap* heap)
{
	for (auto& [k, v] : m_textures)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		auto tex = m_textures[v->Name]->Resource;

		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = tex->GetDesc().Format;
		srvDesc.ViewDimension = m_textures[v->Name]->Dimension;
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

		v->heapIndex=heap->CreateSRV(tex.Get(), srvDesc);
		SaveMaterialData(v->Name, v->heapIndex);
	}
}

void LoadAsset::CreateTexture(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	std::string fileName,
	_Out_ ComPtr<ID3D12Resource>& texture,
	_Out_ ComPtr<ID3D12Resource>& textureUploadHeap)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	const UINT TexturePixelSize = 4;//������ rgb_alpha ������4ͨ��

	D3D12_RESOURCE_DESC textureDesc = {};
	//�ݶ�Ϊ1
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = texWidth;
	textureDesc.Height = texHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	//����szFileName��ʵ���õ�������Դ   
	device->CreateCommittedResource(
		rvalue_to_lvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(texture.GetAddressOf()));

	const UINT num2DSubresources = textureDesc.DepthOrArraySize * textureDesc.MipLevels;
	//�ߴ磺
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, num2DSubresources);

	// Create the GPU upload buffer.
	ThrowIfFailed(device->CreateCommittedResource(
		rvalue_to_lvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		rvalue_to_lvalue(CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(textureUploadHeap.GetAddressOf())));


	//�����ݸ��Ƹ��м���Դ��Ȼ����м���Դ���Ƹ�ʵ�ʵ���Դ
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