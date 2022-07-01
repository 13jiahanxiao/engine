#include"RenderItem.h"

RenderItemManager::RenderItemManager(Device* device, ID3D12GraphicsCommandList* cmdList): mDevice(device)
{
	//加载资源
	mTextureManager= std::make_unique<TextureManager>();
	mTextureManager->LoadTextureXML();

	mTextureHeap = std::make_unique<DescriptorHeap>(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mTextureManager->GetTextureNum()+4, true);

	mGeometryManager = std::make_unique<GeometryManager>(mDevice, cmdList);

	//创建描述符
	mTextureManager->CreateDDSTexture(mDevice, cmdList);
	mTextureManager->BuildTextureHeap(mTextureHeap.get());

	mMaterialManager = std::make_unique<MaterialManager>(gNumFrameResources);
	mMaterialManager->LoadMaterialXML();
}

RenderItemManager::~RenderItemManager() 
{

}

void RenderItemManager::AddRenderItemInLayer(std::string itemName, RenderLayer renderLayer)
{
	auto item = mRitems[itemName].get();
	if(item)
		mRitemLayer[(int)renderLayer].push_back(item);
}

//每个要渲染的物体指定属性
void RenderItemManager::BuildRenderItem(std::string itemName, RenderLayer renderLayer,
	std::string geoName, std::string argName, std::string materialName,
	DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform, D3D_PRIMITIVE_TOPOLOGY topo)
{
	auto Ritem = std::make_unique<RenderItem>();
	//检查名字重复 todo
	Ritem->Name = itemName;
	Ritem->Layer = renderLayer;
	Ritem->World = MathHelper::Identity4x4();
	Ritem->ObjectCBIndex = mObjectCBIndex;
	mObjectCBIndex++;
	Ritem->Geo = mGeometryManager->GetGeo(geoName);
	Ritem->Mat = GetMaterial(materialName);
	Ritem->PrimitiveType = topo;
	Ritem->IndexCount = Ritem->Geo->GetSubMesh(argName).IndexCount;
	Ritem->StartIndexLocation = Ritem->Geo->GetSubMesh(argName).StartIndexLocation;
	Ritem->BaseVertexLocation = Ritem->Geo->GetSubMesh(argName).BaseVertexLocation;
	XMStoreFloat4x4(&Ritem->World, world);
	XMStoreFloat4x4(&Ritem->TexTransform, texTransform);

	mRitemLayer[(int)renderLayer].push_back(Ritem.get());

	//std::move 会把左值变成右值，防止内存的赋值操作，转移后原对象为空
	mRitems[Ritem->Name] = std::move(Ritem);
}

void RenderItemManager::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	//指针数组 每个指针指向ID3D12DescriptorHeap
	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureHeap->GetHeap() };
	int x = _countof(descriptorHeaps);
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void RenderItemManager::SetRootDescriptorTable(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootDescriptorTable(3, mTextureHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart());
}

void RenderItemManager::DrawRenderItems(UINT objCBByteSize , D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress, 
	ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	auto ritems = mRitemLayer[(int)name];
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
	mMaterialManager->UpdateMaterialCBs(currMaterialBuffer);
}

void RenderItemManager::UpdateObjectCBs(UploadBuffer<ObjectConstants>* cb)
{
	for (auto& [k, v] : mRitems)
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