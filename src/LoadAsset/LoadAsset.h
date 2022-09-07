#pragma once
#include"../Resources/FrameResource.h"
#include"../Resources/DescriptorHeap.h"
#include<string>
#include<vector>
#include<unordered_map>
namespace REngine 
{
	struct LoadMaterialData
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.25f;
		UINT DiffuseMapIndex = 0;
	};

	struct Texture
	{
		std::string Name;
		std::string Filename;

		Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

		int heapIndex = 0;

		D3D12_SRV_DIMENSION Dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	};

	struct SubMeshData
	{
		std::string m_name;
		int m_startVertex;
		int m_startIndex;
		int m_indexSize;
		//图片类型名，和图片名
		int m_indexHeap = -1;
		std::string material;
	};

	struct MeshData
	{
		std::string m_name;
		std::vector<Vertex> m_vertices;
		std::vector<std::uint16_t> m_indices;
		std::vector<SubMeshData> m_submesh;
	};

	class LoadAsset
	{
	public:
		LoadAsset();
		~LoadAsset();
		void LoadObjectFromJson();
		void ParseObjectData(std::string geoName, std::string fileName);
		std::vector<MeshData> GetAllMesh() { return m_meshData; }

		std::unordered_map<std::string, LoadMaterialData> GetMaterials() {return m_materials;}
		void SaveMaterialData(std::string name, int index);
		
		void SaveTextureData(std::string fileName, int dimension=4);
		void LoadTextureFromJson();
		//构建描述符堆
		void BuildTextureHeap(DescriptorHeap* heap);
		//创建描述符
		void CreateDDSTexture(Device* device, ID3D12GraphicsCommandList* cmdList);
		void CreateTexture(Device* device, ID3D12GraphicsCommandList* cmdList);
		void CreateTexture(ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			std::string fileName,
			_Out_ ComPtr<ID3D12Resource>& texture,
			_Out_ ComPtr<ID3D12Resource>& textureUploadHeap);
		int GetTextureNum() { return m_textures.size(); }
	private:

		std::vector<MeshData> m_meshData;
		std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
		std::unordered_map<std::string, LoadMaterialData> m_materials;
	};
}


