#pragma once
#include"../DXRuntime/Device.h"
#include"../Resources/DescriptorHeap.h"
#include<string>
#include <d3d12.h>
#include <wrl.h>
#include<unordered_map> 
#include<memory>
struct Texture
{
	std::string Name;

	std::string Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

	int heapIndex = 0;

	D3D12_SRV_DIMENSION Dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
};

class TextureManager
{
public:
	TextureManager();
	~TextureManager();
	void Init();
	//��xml��ȡ���õ�texture��ַ
	void LoadTextureFormJson(std::string name, std::string fileName, int index, int dimension);
	void LoadTextureJson();
	//������������
	void BuildTextureHeap(DescriptorHeap* heap);
	//����������
	void CreateTexture(Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateTexture(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		std::string fileName,
		_Out_ ComPtr<ID3D12Resource>& texture,
		_Out_ ComPtr<ID3D12Resource>& textureUploadHeap);
	int GetTextureNum() { return m_Textures.size(); }
private:
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
};