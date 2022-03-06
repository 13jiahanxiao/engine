#pragma once

#include"../Utility/MathHelper.h"
#include <DirectXMath.h>
#include<string>

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

class Material
{
public:
	Material(std::string name,int index, DirectX::XMFLOAT4 diffuse, DirectX::XMFLOAT3 fresnel,float roughness,int dirtyflag);
	void SetTexture(int dSrvHeapIndex, DirectX::XMFLOAT4X4 matTransform) 
	{
		DiffuseSrvHeapIndex = dSrvHeapIndex;
		MatTransform = matTransform;
	}
	void SetTexture(int dSrvHeapIndex) 
	{
		DiffuseSrvHeapIndex = dSrvHeapIndex;
	}
	void SetMatTransform(DirectX::XMFLOAT4X4 matTransform) 
	{
		MatTransform = matTransform;
	}
	void SetMatTransform(DirectX::XMMATRIX matTransform)
	{
		XMStoreFloat4x4(&MatTransform, matTransform);
	}

	void SetMatTransformValue(int col, int row, float value) 
	{ 
		MatTransform(col, row) = value; 
	}
	float GetMatTransformValue(int col, int row) { return MatTransform(col, row); }
	void SetNumFramesDirty(int i)
	{
		NumFramesDirty = i;
	}

	void UpdateDirtyFlag(int i) { NumFramesDirty += i; }

	int GetDiffuseSrvHeapIndex() { return DiffuseSrvHeapIndex; }
	int GetMatCBIndex() { return MatCBIndex; }
	int GetNumFramesDirty() { return NumFramesDirty; }
	DirectX::XMFLOAT4 GetDiffuseAlbedo() { return DiffuseAlbedo; }
	DirectX::XMFLOAT3 GetFresnel() { return FresnelR0; }
	float GetRoughness(){return Roughness; }
	DirectX::XMFLOAT4X4 GetMatTransform() { return MatTransform; }
	~Material();
private:
	std::string Name;

	int MatCBIndex;

	int DiffuseSrvHeapIndex;

	int NormalSrvHeapIndex ;

	int NumFramesDirty;

	DirectX::XMFLOAT4 DiffuseAlbedo;
	DirectX::XMFLOAT3 FresnelR0 ;
	float Roughness;
	DirectX::XMFLOAT4X4 MatTransform;
};