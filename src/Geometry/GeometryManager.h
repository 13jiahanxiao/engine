#pragma once

#include"MeshGeometry.h"
#include"../LoadAsset/LoadAsset.h"
#include"../Resources/FrameResource.h"

class GeometryManager
{
public:
	GeometryManager(Device* device, ID3D12GraphicsCommandList* cmdList);
	~GeometryManager();
	void CreateMesh(std::string name,std::vector<Vertex> vertices, std::vector<std::uint16_t> indices);
	void CreateGeoMesh(std::string name, std::vector<GeoVertex> vertices, std::vector<std::uint16_t> indices);
	void CreateSubMesh(std::string name,  std::vector<REngine::SubMeshData> m_submesh);
	void CreateSubMesh(std::string name, std::string subName, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
	void CreateMeshVertexUpload(std::string name, UINT vbSize, std::vector<std::uint16_t> indices);
	MeshGeometry* GetGeo(std::string name) { return m_Geometries[name].get(); }
	void LoadMesh(REngine::LoadAsset* LoadAssetManager);
private:
	//Œﬁ–Ú”≥…‰π‹¿Ì
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	Device* m_Device;
	ID3D12GraphicsCommandList* m_CmdList;
};

