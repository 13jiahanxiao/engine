#include"Materials.h"
#include"nlohmann/json.hpp"
using json = nlohmann::json;

Material::Material(std::string name, int index, DirectX::XMFLOAT4 diffuse, DirectX::XMFLOAT3 fresnel, float roughness,int dirtyFlag):
	Name(name),
	MatCBIndex(index),
	DiffuseAlbedo(diffuse),
	FresnelR0(fresnel),
	Roughness(roughness),
	DiffuseSrvHeapIndex(-1),
	NormalSrvHeapIndex(-1),
	NumFramesDirty(dirtyFlag),
	MatTransform(MathHelper::Identity4x4())
{

}

Material::~Material()
{

}

MaterialManager::MaterialManager(int FrameNum):mFrameNum(FrameNum)
{

}
MaterialManager::~MaterialManager() 
{

}

void MaterialManager::CreateMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness)
{
	if (m_materials.find(materialName) != m_materials.end())
		return;
	auto mat = std::make_unique<Material>(materialName, mMaterialCBIndex, diffuseAlbedo, fresnelR0, roughness, mFrameNum);
	mMaterialCBIndex++;
	mat->SetTexture(srvIndex);
	m_materials[materialName] = std::move(mat);
}

void MaterialManager::LoadMaterialFormJson()
{
	std::ifstream f("Resources\\Json\\MaterialConfig.json");
	json data = json::parse(f);
	json mData = data["MaterialConfig"];
	for (int i = 0; i < mData.size(); i++)
	{
		std::string name = mData[i].at("name");
		int srvIndex= mData[i].at("srvIndex");

		std::string temp= mData[i].at("diffuseAlbedo");
		std::vector<std::string> svert;
		Split(temp, svert, ",");
		DirectX::XMFLOAT4 diffuse;
		diffuse.x = atof(svert[0].c_str());
		diffuse.y = atof(svert[1].c_str());
		diffuse.z = atof(svert[2].c_str());
		diffuse.w = atof(svert[3].c_str());

		svert.clear();
		temp = mData[i].at("fresnelR0"); 
		Split(temp, svert, ",");
		DirectX::XMFLOAT3 fresnel;
		fresnel.x = atof(svert[0].c_str());
		fresnel.y = atof(svert[1].c_str());
		fresnel.z = atof(svert[2].c_str());

		float roughness = mData[i].at("roughness");
		CreateMaterial(name, srvIndex, diffuse, fresnel, roughness);
	}
}

void MaterialManager::UpdateMaterialCBs(UploadBuffer<MaterialData>* cb)
{
	for (auto& e : m_materials)
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