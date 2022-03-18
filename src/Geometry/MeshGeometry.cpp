#include"MeshGeometry.h"

MeshGeometry::MeshGeometry(std::string name, UINT vbSize, UINT ibSize) :
	m_Name(name),
	m_VertexBufferCPU(nullptr),
	m_IndexBufferCPU(nullptr),
	m_VertexBufferGPU(nullptr),
	m_IndexBufferGPU(nullptr),
	m_VertexBufferUploader(nullptr),
	m_IndexBufferUploader(nullptr),
	m_IndexFormat(DXGI_FORMAT_R16_UINT)
{
	m_VertexByteStride = sizeof(Vertex);
	m_VertexBufferByteSize = vbSize;
	m_IndexFormat = DXGI_FORMAT_R16_UINT;
	m_IndexBufferByteSize = ibSize;
}

MeshGeometry::~MeshGeometry()
{
}

void MeshGeometry::InitVertex(Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<Vertex> vertices, UINT vbSize)
{
	ThrowIfFailed(D3DCreateBlob(vbSize, &m_VertexBufferCPU));
	CopyMemory(m_VertexBufferCPU->GetBufferPointer(), vertices.data(), vbSize);

	m_VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device->GetDevice(),cmdList, vertices.data(), vbSize, m_VertexBufferUploader);
}

void MeshGeometry::InitIndex(Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<std::uint16_t> indices, UINT ibSize)
{
	ThrowIfFailed(D3DCreateBlob(ibSize, &m_IndexBufferCPU));
	CopyMemory(m_IndexBufferCPU->GetBufferPointer(), indices.data(), ibSize);

	m_IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device->GetDevice(),cmdList, indices.data(), ibSize, m_IndexBufferUploader);
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::VertexBufferView()const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = m_VertexByteStride;
	vbv.SizeInBytes = m_VertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::IndexBufferView()const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = m_IndexFormat;
	ibv.SizeInBytes = m_IndexBufferByteSize;

	return ibv;
}

void MeshGeometry::DisposeUploaders()
{
	m_VertexBufferUploader = nullptr;
	m_IndexBufferUploader = nullptr;
}