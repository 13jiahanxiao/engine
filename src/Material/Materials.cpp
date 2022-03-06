#include"Materials.h"

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