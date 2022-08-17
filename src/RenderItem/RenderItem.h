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
    Mirror,
    Reflection,
    AlphaTestedAndRefection,
    BillBoardTree,
    VertBlur,
    HorzBlur,
    Sky,
    Count,
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

    void Init();

    void LoadRenderItemFromJson();
   //��Ⱦ��
   void BuildRenderItem(std::string itemName, RenderLayer renderLayer,
       std::string geoName, std::string argName, std::string materialName,
       DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform, D3D_PRIMITIVE_TOPOLOGY topo= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   void DrawRenderItems(UINT objCBByteSize, D3D12_GPU_VIRTUAL_ADDRESS objCBGPUAddress,
       ID3D12GraphicsCommandList* cmdList, RenderLayer name);
   //���»�����
   void UpdateCBs(FrameResource* currentFrameResource);
   void UpdateObjectCBs(UploadBuffer<ObjectConstants>* cb);
   
   void AddRenderItemInLayer(std::string itemName, RenderLayer renderLayer);

   void BuildAllSubRenderItem(std::string itemName, RenderLayer renderLayer, std::string geoName,
       DirectX::XMMATRIX world, DirectX::XMMATRIX texTransform, D3D_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   int ItemsSize() { return mRitems.size(); }

   Material* GetMaterial(std::string name) { return mMaterialManager->GetMaterial(name); }
   RenderItem* GetRenderItem(std::string name) { return mRitems[name].get(); }

   GeometryManager* GetMeshManager() {return mGeometryManager.get();}

   int GetMaterilalsNum() { return  mMaterialManager->MaterilalsSize(); }
private:
    Device* mDevice;

    std::unique_ptr <MaterialManager> mMaterialManager;

    std::unordered_map<std::string, std::unique_ptr<RenderItem>> mRitems;

    std::unique_ptr <GeometryManager> mGeometryManager;

    //��Ⱦ�㼶����
    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    //��Ⱦ�������
    int mObjectCBIndex = 0;
};