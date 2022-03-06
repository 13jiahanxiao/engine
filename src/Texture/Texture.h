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
	//从xml读取配置的texture地址
	void LoadTextureFormXML(std::string name, std::string fileName, int index);
	void LoadTextureXML();
	//构建描述符堆
	void BuildTextureHeap(DescriptorHeap* heap);
	//创建描述符
	void LoadTexture(Device* device, ID3D12GraphicsCommandList* cmdList);
private:
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
};