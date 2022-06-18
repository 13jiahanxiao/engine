#pragma once
#include"../Material/Materials.h"
#include"../Texture/Texture.h"
#include"../Resources/DescriptorHeap.h"
#include"../Resources/FrameResource.h"
#include"../Geometry/MeshGeometry.h"
#include"../Geometry/GeometryManager.h"
#include <DirectXMath.h>
extern const int gNumFrameResources;

enum  class  RenderLayer : int
{
    Opaque = 0,
    Transparent,
    AlphaTested,
    CullBack,
    TexRotate,
    NoTexture,
    Wireframe,
    Count
};

struct RenderItem
{
    RenderItem() = default;

    std::string Name="default";

    RenderLayer Layer = RenderLayer::Opaque;

    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

    DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    //帧数量
    int NumFramesDirty = gNumFrameResources;
    //缓存区下标
    UINT ObjectCBIndex = -1;
    //材质
    Material* Mat = nullptr;
    //几何体
    MeshGeometry* Geo = nullptr;
    //拓扑结构
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class RenderItemManager 
{
public:
    //初始化texture的管理
    RenderItemManager(Device* device,ID3D12GraphicsCommandList* cmdList);
    ~RenderItemManager();
    //构建材质
   void BuildMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness);
   //渲染项
   void BuildRenderItem(std::string itemName, RenderLayer renderLayer,
       std::string geoName, std::string argName, std::string materialName,
       DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform);

   void DrawRenderItems(UINT objCBByteSize, D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress,
       ID3D12GraphicsCommandList* cmdList, RenderLayer name);

   //绑定描述符到list
   void SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList);
   void SetRootDescriptorTable(ID3D12GraphicsCommandList* cmdList);
   //更新缓冲区
   void UpdateCBs(FrameResource* currentFrameResource);
   void UpdateObjectCBs(UploadBuffer<ObjectConstants>* cb);
   void UpdateMaterialCBs(UploadBuffer<MaterialData>* cb);
   
   int MaterialsSize() { return m_Materials.size(); }
   int ItemsSize() { return m_Ritems.size(); }

  Material* GetMaterial(std::string name) { return m_Materials[name].get(); }
  RenderItem* GetRenderItem(std::string name) { return m_Ritems[name].get(); }

  GeometryManager* GetMeshManager() {return m_GeometryManager.get();}
private:
    Device* m_Device;

	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;

    std::unique_ptr <TextureManager> m_TextureManager;
    //绑定贴图资源的heap
    std::unique_ptr<DescriptorHeap> m_TextureHeap;

    std::unordered_map<std::string, std::unique_ptr<RenderItem>> m_Ritems;

    std::unique_ptr <GeometryManager> m_GeometryManager;

    //渲染层级管理
    std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];

    //材质索引
    int m_MaterialCBIndex = 0;
    //渲染项的索引
    int m_ObjectCBIndex = 0;
};