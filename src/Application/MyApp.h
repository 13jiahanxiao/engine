#include "d3dApp.h"
#include "../Resources/FrameResource.h"
#include	"../Wave/Wave.h"
#include"../Resources/DescriptorHeap.h"
#include"../Material/Materials.h"
#include "../Components/Camera.h"
#include"../Texture/Texture.h"
#include"../RenderItem/RenderItem.h"
#include"../Utility/ShaderCompile.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class MyApp : public D3DApp
{
public:
    MyApp(HINSTANCE hInstance);
    MyApp(const MyApp& rhs) = delete;
    MyApp& operator=(const MyApp& rhs) = delete;
    ~MyApp();

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

    void BuildRootSignature();
    void BuildShadersAndInputLayout();

    void BuildMaterials();

    //����
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
    FXMMATRIX PositionMatrix(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 translate);
	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;
private:
	//֡��Դ
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    
	//����ӳ�����
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	PassConstants mMainPassCB;

	std::unique_ptr<Waves> m_Waves;
	RenderItem* m_WavesRitem = nullptr;

    UINT mPassCbvOffset = 0;

    bool mIsWireframe = false;

    Camera m_Camera;
     
    DirectionLight m_SunLight;

    std::unique_ptr<RenderItemManager> m_ItemManager;

    std::unique_ptr<ShaderCompile> m_Shaders;

    POINT mLastMousePos;
};