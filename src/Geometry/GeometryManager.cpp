#include"GeometryManager.h"
#include"LoadObject.h"

GeometryManager::GeometryManager(Device* device, ID3D12GraphicsCommandList* cmdList):
	m_Device(device),m_CmdList(cmdList)
{
}

GeometryManager::~GeometryManager()
{
}

void GeometryManager::CreateMesh(std::string name,std::vector<Vertex> vertices, std::vector<std::uint16_t> indices)
{
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	auto geo = std::make_unique<MeshGeometry>(name,vbByteSize,ibByteSize);
	geo->InitVertex(m_Device, m_CmdList, vertices, vbByteSize);
	geo->InitIndex(m_Device, m_CmdList, indices, ibByteSize);
	m_Geometries[name] = std::move(geo);
}

void GeometryManager::CreateSubMesh(std::string name, std::string subName, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) 
{
	SubmeshGeometry sub;
	sub.BaseVertexLocation = BaseVertexLocation;
	sub.IndexCount = IndexCount;
	sub.StartIndexLocation = StartIndexLocation;
	m_Geometries[name]->AddSubMesh(subName, sub);
}

void GeometryManager::CreateMeshVertexUpload(std::string name, UINT vbSize, std::vector<std::uint16_t> indices)
{
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	auto geo = std::make_unique<MeshGeometry>(name, vbSize, ibByteSize);
	geo->InitIndex(m_Device, m_CmdList, indices, ibByteSize);
	m_Geometries[name] = std::move(geo);
}

void GeometryManager::LoadMesh(std::string fileName, std::string geoName,std::string subName)
{
	lo.LoadOBJ(fileName);
	CreateMesh(geoName, lo.m_Vertices, lo.m_Indices);
	CreateSubMesh(geoName, subName, (UINT)lo.m_Indices.size(), 0, 0);
}


void GeometryManager::CreateGeoMesh(std::string name, std::vector<GeoVertex> vertices, std::vector<std::uint16_t> indices)
{
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	auto geo = std::make_unique<MeshGeometry>(name, vbByteSize, ibByteSize);
	geo->InitGeoVertex(m_Device, m_CmdList, vertices, vbByteSize);
	geo->InitIndex(m_Device, m_CmdList, indices, ibByteSize);
	m_Geometries[name] = std::move(geo);
}