#include "D3DScene.h"
#include"../LoadAsset/LoadAsset.h"
#include"../Geometry/MeshGeometry.h"
#include "../Resources/FrameResource.h"
#include	"../Wave/Wave.h"
#include"../Resources/DescriptorHeap.h"
#include"../Material/Materials.h"
#include"../Geometry/GeometryManager.h"
#include"../PostProcess/BlurFilter.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class DefaultScene : public D3DScene
{
public:
    DefaultScene(HINSTANCE hInstance);
    DefaultScene(const DefaultScene& rhs) = delete;
    DefaultScene& operator=(const DefaultScene& rhs) = delete;
    ~DefaultScene();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateReflectPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
    void UpdateLight(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateCBs(FrameResource* currentFrameResource);

    void UpdateObjectCBs(UploadBuffer<ObjectConstants>* cb);

    void DrawItems();
    //����
    void BuildSkullGeometry();
    void BuildGeometrys();
    void LandGeometry();
	void WavesGeometry();
    void ShapeGeometry();
    void BillTreeGeometry();
    //��������
    void BuildPSOs();

	//֡��Դ�ʹ���
    void BuildFrameResources();

    //Ϊ��Ⱦ��ָ������
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name);
    void DrawItemByPsoLayer(RenderLayer renderLayer);

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;
private:
	//֡��Դ
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;
    
	PassConstants mMainPassCB;
    PassConstants mReflectPassCB;

	std::unique_ptr<Waves> m_Waves;
	RenderItem* m_WavesRitem = nullptr;

    UINT mPassCbvOffset = 0;

    bool mIsWireframe = false;
     
    DirectionLight m_SunLight;
    //����ͼ��Դ��heap
    std::unique_ptr<DescriptorHeap> mTextureHeap;

    std::unique_ptr<REngine::LoadAsset> LoadAssetManager;
};