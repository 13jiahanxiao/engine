#include"RenderItem.h"

RenderItemManager::RenderItemManager(int textureHeapNum, Device* device, ID3D12GraphicsCommandList* cmdList): m_Device(device)
{
	m_TextureHeap = std::make_unique<DescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureHeapNum, true);
	m_TextureManager= std::make_unique<TextureManager>();

	m_TextureManager->LoadTextureXML();

	m_GeometryManager = std::make_unique<GeometryManager>(m_Device, cmdList);

	//����������
	m_TextureManager->LoadTexture(m_Device, cmdList);
	m_TextureManager->BuildTextureHeap(m_TextureHeap.get());
}

RenderItemManager::~RenderItemManager() 
{

}

//����,����,ͼƬ��Դ����,����,������,�ֲڶ�
void RenderItemManager::BuildMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness)
{
	auto mat = std::make_unique<Material>(materialName, m_MaterialCBIndex, diffuseAlbedo, fresnelR0, roughness, gNumFrameResources);
	m_MaterialCBIndex++;
	mat->SetTexture(srvIndex);
	m_Materials[materialName] = std::move(mat);
}

//ÿ��Ҫ��Ⱦ������ָ������
void RenderItemManager::BuildRenderItem(std::string itemName, RenderLayer renderLayer,
	std::string geoName, std::string argName, std::string materialName,
	DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform)
{
	auto Ritem = std::make_unique<RenderItem>();
	//��������ظ� todo
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

	//std::move �����ֵ�����ֵ����ֹ�ڴ�ĸ�ֵ������ת�ƺ�ԭ����Ϊ��
	m_Ritems[Ritem->Name] = std::move(Ritem);
}

void RenderItemManager::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	//ָ������ ÿ��ָ��ָ��ID3D12DescriptorHeap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_TextureHeap->GetHeap() };
	int x = _countof(descriptorHeaps);
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void RenderItemManager::DrawRenderItems(UINT objCBByteSize , D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress, 
	UINT matCBByteSize, D3D12_GPU_VIRTUAL_ADDRESS matCBGPUAddress,
	ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	auto ritems = m_RitemLayer[name];
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, rvalue_to_lvalue(ri->Geo->VertexBufferView()));
		cmdList->IASetIndexBuffer(rvalue_to_lvalue(ri->Geo->IndexBufferView()));
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCBGPUAddress + ri->ObjectCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCBGPUAddress + ri->Mat->GetMatCBIndex() * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, m_TextureHeap->hGPU(ri->Mat->GetDiffuseSrvHeapIndex()));
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void RenderItemManager::UpdateCBs(FrameResource* currentFrameResource)
{
	auto currObjectCB = currentFrameResource->ObjectCB.get();
	UpdateObjectCBs(currObjectCB);
	auto currMaterialCB = currentFrameResource->MaterialCB.get();
	UpdateMaterialCBs(currMaterialCB);
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

			cb->CopyData(v->ObjectCBIndex, objConstants);

			v->NumFramesDirty--;
		}
	}
}

void RenderItemManager::UpdateMaterialCBs(UploadBuffer<MaterialConstants>* cb)
{
	for (auto& e : m_Materials)
	{
		Material* mat = e.second.get();
		if (mat->GetNumFramesDirty() > 0)
		{
			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->GetDiffuseAlbedo();
			matConstants.FresnelR0 = mat->GetFresnel();
			matConstants.Roughness = mat->GetRoughness();
			matConstants.MatTransform = mat->GetMatTransform();

			cb->CopyData(mat->GetMatCBIndex(), matConstants);

			mat->UpdateDirtyFlag(-1);
		}
	}
}