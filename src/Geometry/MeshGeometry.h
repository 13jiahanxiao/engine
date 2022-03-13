#pragma once
#include"../Utility/d3dUtil.h"
#include"../Resources/FrameResource.h"
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	// This is used in later chapters of the book.
	DirectX::BoundingBox Bounds;
};

class MeshGeometry
{
public:
	MeshGeometry(std::string name, UINT vbSize, UINT ibSize);
	~MeshGeometry();

	void InitVertex(Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<Vertex> vertices, UINT vbSize);
	void InitIndex(Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<std::uint16_t> indices, UINT ibSize);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const;

	void DisposeUploaders();

	void AddSubMesh(std::string name, SubmeshGeometry sub) { m_DrawArgs[name] = sub; }
	SubmeshGeometry  GetSubMesh(std::string name)  { return m_DrawArgs[name]; }

	void SetVertexBufferGPU(ID3D12Resource* resource) { m_VertexBufferGPU = resource; }
private:
	std::string m_Name;

	Microsoft::WRL::ComPtr<ID3DBlob> m_VertexBufferCPU ;
	Microsoft::WRL::ComPtr<ID3DBlob> m_IndexBufferCPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBufferGPU ;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBufferGPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBufferUploader ;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBufferUploader;

	UINT m_VertexByteStride ;
	UINT m_VertexBufferByteSize ;
	DXGI_FORMAT m_IndexFormat ;
	UINT m_IndexBufferByteSize ;

	std::unordered_map<std::string, SubmeshGeometry> m_DrawArgs;
};