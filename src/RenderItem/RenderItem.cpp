#include"RenderItem.h"
#include"nlohmann/json.hpp"
#include<DirectXMath.h>
using json = nlohmann::json;
using namespace REngine;
RenderItemManager::RenderItemManager(Device* device, ID3D12GraphicsCommandList* cmdList): mDevice(device)
{
	mGeometryManager = std::make_unique<GeometryManager>(mDevice, cmdList);

	mMaterialManager = std::make_unique<MaterialManager>(gNumFrameResources);
}

void RenderItemManager::CreateMaterial(REngine::LoadAsset* LoadAssetManager)
{
	auto pMaterials = LoadAssetManager->GetMaterials();
	auto it = pMaterials.begin();
	while (it != pMaterials.end())
	{
		mMaterialManager->CreateMaterial(it->first, it->second.DiffuseMapIndex, it->second.DiffuseAlbedo,
			it->second.FresnelR0, it->second.Roughness);
		it++;
	}
}

void RenderItemManager::LoadRenderItemFromJson()
{
	std::ifstream f("Resources\\Json\\RenderItems.json");
	json data = json::parse(f);
	json mData = data["RenderItems"];
	//加载读出的物体
	for (int i = 0; i < mData.size(); i++)
	{
		std::string itemName = mData[i].at("itemName");
		int renderLayer = mData[i].at("renderLayer");
		std::string geoName = mData[i].at("geoName");

		std::string temp = mData[i].at("world");
		std::vector<std::string> svert;
		Split(temp, svert, ",");
		DirectX::FXMMATRIX world = MathHelper::PositionMatrix(atof(svert[0].c_str()), atof(svert[1].c_str()), atof(svert[2].c_str()),
			atof(svert[3].c_str()), atof(svert[4].c_str()), atof(svert[5].c_str()), atof(svert[6].c_str()));
		svert.clear();

		temp = mData[i].at("texTransform");
		Split(temp, svert, ",");
		DirectX::FXMMATRIX tex = MathHelper::PositionMatrix(atof(svert[0].c_str()), atof(svert[1].c_str()), atof(svert[2].c_str()),
			atof(svert[3].c_str()), atof(svert[4].c_str()), atof(svert[5].c_str()), atof(svert[6].c_str()));
		
		BuildAllSubRenderItem(itemName, (RenderLayer)renderLayer, geoName,world, tex);
	}

	//单独构造物体
	//json mData = data["RenderItem"];
	//for (int i = 0; i < mData.size(); i++)
	//{
	//	std::string itemName = mData[i].at("itemName");
	//	int renderLayer = mData[i].at("renderLayer");
	//	std::string geoName = mData[i].at("geoName");
	//	std::string argName = mData[i].at("argName");
	//	std::string materialName = mData[i].at("materialName");
	//	std::string temp = mData[i].at("world");
	//	std::vector<std::string> svert;
	//	Split(temp, svert, ",");
	//	DirectX::FXMMATRIX world=  MathHelper::PositionMatrix(atof(svert[0].c_str()), atof(svert[1].c_str()), atof(svert[2].c_str()), 
	//		atof(svert[3].c_str()),atof(svert[4].c_str()),atof(svert[5].c_str()),atof(svert[6].c_str()));
	//	svert.clear();
	//	temp = mData[i].at("texTransform");
	//	Split(temp, svert, ",");
	//	DirectX::FXMMATRIX tex= MathHelper::PositionMatrix(atof(svert[0].c_str()), atof(svert[1].c_str()), atof(svert[2].c_str()),
	//		atof(svert[3].c_str()), atof(svert[4].c_str()), atof(svert[5].c_str()), atof(svert[6].c_str()));
	//	int topology = mData[i].at("topology");
	//	BuildRenderItem(itemName, (RenderLayer)renderLayer, geoName, argName, materialName, world, tex, (D3D_PRIMITIVE_TOPOLOGY)topology);
	//}
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

void RenderItemManager::BuildAllSubRenderItem(std::string itemName, RenderLayer renderLayer, std::string geoName,
	DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform, D3D_PRIMITIVE_TOPOLOGY topo)
{
	auto geo=mGeometryManager->GetGeo(geoName);
	auto pSub = geo->GetAllSubMesh();
	auto it = pSub.begin();
	//it = pSub.find("eyewhite");
	//if (it != pSub.end()) 
	//{
	//	BuildRenderItem(itemName, renderLayer, geoName, it->first, it->second.m_material, world, texTransform, topo);
	//}
	int count = 0;
	while (it != pSub.end()) 
	{
		count++;
		std::string name = itemName;
		name += std::to_string(count);
		BuildRenderItem(name, renderLayer, geoName, it->first, it->second.material,world, texTransform, topo);
		it++;
	}
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