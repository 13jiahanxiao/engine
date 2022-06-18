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
    //֡����
    int NumFramesDirty = gNumFrameResources;
    //�������±�
    UINT ObjectCBIndex = -1;
    //����
    Material* Mat = nullptr;
    //������
    MeshGeometry* Geo = nullptr;
    //���˽ṹ
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class RenderItemManager 
{
public:
    //��ʼ��texture�Ĺ���
    RenderItemManager(Device* device,ID3D12GraphicsCommandList* cmdList);
    ~RenderItemManager();
    //��������
   void BuildMaterial(std::string materialName, int srvIndex, DirectX::XMFLOAT4 diffuseAlbedo, DirectX::XMFLOAT3 fresnelR0, float roughness);
   //��Ⱦ��
   void BuildRenderItem(std::string itemName, RenderLayer renderLayer,
       std::string geoName, std::string argName, std::string materialName,
       DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform);

   void DrawRenderItems(UINT objCBByteSize, D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress,
       ID3D12GraphicsCommandList* cmdList, RenderLayer name);

   //����������list
   void SetDescriptorHeaps(ID3D12GraphicsCommandList* cmdList);
   void SetRootDescriptorTable(ID3D12GraphicsCommandList* cmdList);
   //���»�����
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
    //����ͼ��Դ��heap
    std::unique_ptr<DescriptorHeap> m_TextureHeap;

    std::unordered_map<std::string, std::unique_ptr<RenderItem>> m_Ritems;

    std::unique_ptr <GeometryManager> m_GeometryManager;

    //��Ⱦ�㼶����
    std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];

    //��������
    int m_MaterialCBIndex = 0;
    //��Ⱦ�������
    int m_ObjectCBIndex = 0;
};