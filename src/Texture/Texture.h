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

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

	int heapIndex = 0;
};

class TextureManager
{
public:
	TextureManager();
	~TextureManager();
	//��xml��ȡ���õ�texture��ַ
	void LoadTextureFormXML(std::string name, std::string fileName, int index);
	void LoadTextureXML();
	//������������
	void BuildTextureHeap(DescriptorHeap* heap);
	//����������
	void LoadTexture(Device* device, ID3D12GraphicsCommandList* cmdList);
private:
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
};