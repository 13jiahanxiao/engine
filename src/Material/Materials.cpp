#include"Materials.h"

void Split(const std::string& in, std::vector<std::string>& out, std::string token)
{
	out.clear();
	std::string temp;
	for (int i = 0; i < int(in.size()); i++)
	{
		std::string test = in.substr(i, token.size());

		if (test == token)
		{
			if (!temp.empty())
			{
				out.push_back(temp);
				temp.clear();
				i += (int)token.size() - 1;
			}
			else
			{
				out.push_back("");
			}
		}
		else if (i + token.size() >= in.size())
		{
			temp += in.substr(i, token.size());
			out.push_back(temp);
			break;
		}
		else
		{
			temp += in[i];
		}
	}
}

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

void MaterialManager::Init()
{
	LoadMaterialXML();
}

void MaterialManager::BuildMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness)
{
	auto mat = std::make_unique<Material>(materialName, mMaterialCBIndex, diffuseAlbedo, fresnelR0, roughness, mFrameNum);
	mMaterialCBIndex++;
	mat->SetTexture(srvIndex);
	mMaterials[materialName] = std::move(mat);
}

void MaterialManager::LoadMaterialXML()
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile("Resources\\XML\\materialConfig.xml");

	XMLElement* materialHanlde = doc.FirstChildElement("material");
	if (materialHanlde)
	{
		XMLElement* item = materialHanlde->FirstChildElement("Item");
		const char* sztext = NULL;
		while (item)
		{
			sztext = item->Attribute("name");
			std::string name(sztext);
			sztext = item->Attribute("srvIndex");
			int srvIndex = atoi(sztext);

			sztext = item->Attribute("diffuseAlbedo");
			std::vector<std::string> svert;
			Split(sztext, svert, ",");
			DirectX::XMFLOAT4 diffuse;
			diffuse.x= atof(svert[0].c_str());
			diffuse.y = atof(svert[1].c_str());
			diffuse.z = atof(svert[2].c_str());
			diffuse.w = atof(svert[3].c_str());

			svert.clear();
			sztext = item->Attribute("fresnelR0");
			Split(sztext, svert, ",");
			DirectX::XMFLOAT3 fresnel;
			fresnel.x = atof(svert[0].c_str());
			fresnel.y = atof(svert[1].c_str());
			fresnel.z = atof(svert[2].c_str());

			sztext = item->Attribute("roughness");
			float roughness= atof(sztext);
			BuildMaterial(name, srvIndex,diffuse,fresnel,roughness);

			item = item->NextSiblingElement("Item");
		}
	}
}

void MaterialManager::UpdateMaterialCBs(UploadBuffer<MaterialData>* cb)
{
	for (auto& e : mMaterials)
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