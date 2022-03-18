#include"RenderItem.h"

RenderItemManager::RenderItemManager(int textureHeapNum, Device* device, ID3D12GraphicsCommandList* cmdList): m_Device(device)
{
	m_TextureHeap = std::make_unique<DescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureHeapNum, true);
	m_TextureManager= std::make_unique<TextureManager>();

	m_TextureManager->LoadTextureXML();

	m_GeometryManager = std::make_unique<GeometryManager>(m_Device, cmdList);

	//创建描述符
	m_TextureManager->LoadTexture(m_Device, cmdList);
	m_TextureManager->BuildTextureHeap(m_TextureHeap.get());
}

RenderItemManager::~RenderItemManager() 
{

}

//种类,索引,图片资源索引,反射,菲涅尔,粗糙度
void RenderItemManager::BuildMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness)
{
	auto mat = std::make_unique<Material>(materialName, m_MaterialCBIndex, diffuseAlbedo, fresnelR0, roughness, gNumFrameResources);
	m_MaterialCBIndex++;
	mat->SetTexture(srvIndex);
	m_Materials[materialName] = std::move(mat);
}

//每个要渲染的物体指定属性
void RenderItemManager::BuildRenderItem(std::string itemName, RenderLayer renderLayer,
	std::string geoName, std::string argName, std::string materialName,
	DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform)
{
	auto Ritem = std::make_unique<RenderItem>();
	//检查名字重复 todo
	Ritem->Name = itemName;
	Ritem->Layer = renderLayer;
	Ritem->World = MathHelper::Identity4x4();
	Ritem->ObjectCBIndex = m_ObjectCBIndex;
	m_ObjectCBIndex++;
	Ritem->Geo = m_GeometryManager->GetGeo(geoName);
	Ritem->Mat = m_Materials[materialName].get();
	Ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Ritem->IndexCount = Ritem->Geo->GetSubMesh(argName).IndexCount;
	Ritem->StartIndexLocation = Ritem->Geo->GetSubMesh(argName).StartIndexLocation;
	Ritem->BaseVertexLocation = Ritem->Geo->GetSubMesh(argName).BaseVertexLocation;
	XMStoreFloat4x4(&Ritem->World, world);
	XMStoreFloat4x4(&Ritem->TexTransform, texTransform);

	m_RitemLayer[(int)renderLayer].push_back(Ritem.get());

	//std::move 会把左值变成右值，防止内存的赋值操作，转移后原对象为空
	m_Ritems[Ritem->Name] = std::move(Ritem);
}

void RenderItemManager::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	//指针数组 每个指针指向ID3D12DescriptorHeap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_TextureHeap->GetHeap() };
	int x = _countof(descriptorHeaps);
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void RenderItemManager::SetRootDescriptorTable(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootDescriptorTable(3, m_TextureHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart());
}

void RenderItemManager::DrawRenderItems(UINT objCBByteSize , D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress, 
	ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	auto ritems = m_RitemLayer[(int)name];
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, rvalue_to_lvalue(ri->Geo->VertexBufferView()));
		cmdList->IASetIndexBuffer(rvalue_to_lvalue(ri->Geo->IndexBufferView()));
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCBGPUAddress + ri->ObjectCBIndex * objCBByteSize;

		//cmdList->SetGraphicsRootDescriptorTable(0, m_TextureHeap->hGPU(ri->Mat->GetDiffuseSrvHeapIndex()));
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void RenderItemManager::UpdateCBs(FrameResource* currentFrameResource)
{
	auto currObjectCB = currentFrameResource->ObjectCB.get();
	UpdateObjectCBs(currObjectCB);
	auto currMaterialBuffer = currentFrameResource->MaterialBuffer.get();
	UpdateMaterialCBs(currMaterialBuffer);
}

void RenderItemManager::UpdateObjectCBs(UploadBuffer<ObjectConstants>* cb)
{
	for (auto& [k, v] : m_Ritems)
	{
		if (v->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&v->World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&v->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = v->Mat->GetMatCBIndex();

			cb->CopyData(v->ObjectCBIndex, objConstants);

			v->NumFramesDirty--;
		}
	}
}

void RenderItemManager::UpdateMaterialCBs(UploadBuffer<MaterialData>* cb)
{
	for (auto& e : m_Materials)
	{
		Material* mat = e.second.get();
		if (mat->GetNumFramesDirty() > 0)
		{
			MaterialData matData;
			matData.DiffuseAlbedo = mat->GetDiffuseAlbedo();
			matData.FresnelR0 = mat->GetFresnel();
			matData.Roughness = mat->GetRoughness();
			matData.MatTransform = mat->GetMatTransform();
			matData.DiffuseMapIndex = mat->GetDiffuseSrvHeapIndex();

			cb->CopyData(mat->GetMatCBIndex(), matData);

			mat->UpdateDirtyFlag(-1);
		}
	}
}