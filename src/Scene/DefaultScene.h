#include "D3DScene.h"
#include"../Geometry/MeshGeometry.h"
#include "../Resources/FrameResource.h"
#include	"../Wave/Wave.h"
#include"../Resources/DescriptorHeap.h"
#include"../Material/Materials.h"
#include"../Geometry/GeometryManager.h"

const int textureHeapNum = 11;
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
	void UpdateWaves(const GameTimer& gt);
    void UpdateLight(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);

    void DrawItems();

    void BuildRootSignature();

    //����
    void BuildSkullGeometry();
    void BuildGeometrys();
    void LandGeometry();
	void WavesGeometry();
    void ShapeGeometry();
    //��������
    void BuildPSOs();

	//֡��Դ�ʹ���
    void BuildFrameResources();

    //Ϊ��Ⱦ��ָ������
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name);

    //�оٲ�����ģʽ
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
    //�����ź���תָ����һ������
    FXMMATRIX PositionMatrix(float scaleX = 1.0f, float scaleY = 1.0f, float scaleZ = 1.0f,
        float  translateX = 0.0f, float translateY = 0.0f, float translateZ = 0.0f,
        float rotationZ = 0.0f);
	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;
private:
	//֡��Դ
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;
    
	PassConstants mMainPassCB;

	std::unique_ptr<Waves> m_Waves;
	RenderItem* m_WavesRitem = nullptr;

    UINT mPassCbvOffset = 0;

    bool mIsWireframe = false;
     
    DirectionLight m_SunLight;
};