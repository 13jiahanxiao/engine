#pragma once
#include"RenderItem.h"
#include"../Utility/d3dx12.h"
#include<map>
class ShaderCompile;


//��ɫ����Դ������
enum ShaderResourceType
{
	UniformBuffer,
	Texture,
	Sampler,
};

//�����������Ե����࣬������HLSL������
enum VertexInputAttributeType
{
	VIA_POSITION,	//����λ��
	VIA_NORMAL,		//���㷨��
	VIA_TANGENT,	//��������
	VIA_COLOR,		//������ɫ
	VIA_TEXCOORD,	//��ͼUV����
};

//����һ����ɫ����Դ������UniformBuffer����������
class CommonShaderResource
{
};

//һ����ɫ����Դ�Ĳ�λ��Ϣ
struct ShaderResourceSlotInfo
{
	ShaderResourceType Type;	//��Դ����
	std::string Name;				//��Դ����
	unsigned int BindingIndex;		//�󶨵����
	ShaderResourceSlotInfo(ShaderResourceType InType, std::string InName, unsigned int InBindingIndex)
	{
		Type = InType;
		Name = InName;
		BindingIndex = InBindingIndex;
	}
};

//����һ��ͼ�ι��ߵ���Ϣ
struct GraphicsPipelineInfo
{
	std::string VertexShaderFile;	//������ɫ�����ļ���
	std::string PixelShaderFile;	//������ɫ�����ļ���
	std::vector<VertexInputAttributeType> VertexInputAttributeTypes;//�����������Ե�����
	std::vector<ShaderResourceSlotInfo> ShaderResourceSlots;	//��ɫ����Դ�Ĳ�λ��������������
};

//����һ���������ԣ��������Ƕ������Ե����������ռfloat�ĸ���
struct VertexInputAttributeDescription
{
	VertexInputAttributeType Type;	//����
	unsigned int Float32Cout;		//��������ж��ٸ�Float
	VertexInputAttributeDescription(VertexInputAttributeType InType)
	{
		Type = InType;

		//�ڴ����������Float32Cout���Զ������ã�
		switch (Type)
		{
		case VIA_POSITION:
			Float32Cout = 3;	//XYZ��������
			break;
		case VIA_NORMAL:
			Float32Cout = 3;	//XYZ��������
			break;
		case VIA_TANGENT:
			Float32Cout = 3;	//XYZ��������
			break;
		case VIA_COLOR:
			Float32Cout = 4;	//RGBA�ĸ�ͨ��
			break;
		case VIA_TEXCOORD:
			Float32Cout = 2;	//UV��������
			break;
		default:
			break;
		}
	}
};

//�������ԵĴ�ȡ��
class VertexAttributeAccessor
{
	//�����������ݵ�ָ�룺
	std::vector<float>* pVertexAttributeData;
	//ÿ�������ж��ٸ�Float
	int VertexFloatAmount = 0;
	//ÿ�����Զ�Ӧ��Floatƫ��
	std::map<VertexInputAttributeType, int> AttributeTypeOffset;
public:
	VertexAttributeAccessor(const std::vector<VertexInputAttributeDescription>& Layout
		, std::vector<float>* InVertexAttributeData)
	{
		pVertexAttributeData = InVertexAttributeData;

		for (int a = 0; a < Layout.size(); a++)
		{
			AttributeTypeOffset[Layout[a].Type] = VertexFloatAmount;//��¼ƫ��
			VertexFloatAmount += Layout[a].Float32Cout;				//������Ŀ
		}
	}

	//��������
	void Set(int VertexIndex,					//��������
		VertexInputAttributeType AttributeType,	//��������
		int direction,							//����
		float value								//ֵ
	)
	{
		(*pVertexAttributeData)[VertexIndex * VertexFloatAmount + AttributeTypeOffset[AttributeType] + direction] = value;
	}

	//�������
	float Get(int VertexIndex,					//��������
		VertexInputAttributeType AttributeType,	//��������
		int direction							//����
	)
	{
		return (*pVertexAttributeData)[VertexIndex * VertexFloatAmount + AttributeTypeOffset[AttributeType] + direction];
	}
};

//�����Ĺ���״̬���󣬲�ͬ��ͼ��APIӦ�����в�ͬ�ļ̳в���������Ҫ�ĳ�Ա
class CommonPipelineStateObject
{
protected:
	//����ʱ�ṩ�Ĺ�����Ϣ
	GraphicsPipelineInfo CommonInfo;

public:
	//�������벼��
	std::vector<VertexInputAttributeDescription> VertexInputAttributes;

	//��ǰ����Դ����Ӧ��ShaderResourceSlots
	std::vector<CommonShaderResource*> CurrentShaderResources;


	//���캯��
	CommonPipelineStateObject(GraphicsPipelineInfo Info)
	{
		CommonInfo = Info;

		//�Զ����ö����������Ե�����
		for (int i = 0; i < Info.VertexInputAttributeTypes.size(); i++)
			VertexInputAttributes.push_back(VertexInputAttributeDescription(Info.VertexInputAttributeTypes[i]));

		//��ǰ����Դ��ʼΪ��
		for (int i = 0; i < Info.ShaderResourceSlots.size(); i++)
			CurrentShaderResources.push_back(nullptr);
	}

	const GraphicsPipelineInfo* GetCommonInfo() { return &CommonInfo; }

protected://�ڲ�����

	//�趨��Դ������Ӧ�ü̳�����
	//���ڴ�ͳͼ��API����ֱ�ӽ���Դ�󶨵���Ⱦ��������
	//�����Ƚ�ͼ��API��ֻ�Ǹ��߹������������Դ��ʲô�������İ���Ҫ��֮��ʹ�á����
	virtual void InternelSetResource(unsigned int ResourceIndex, CommonShaderResource* InResource)
	{
		CurrentShaderResources[ResourceIndex] = InResource;
	}

public://�ⲿ�ӿ�

	//ͨ�������趨��Դ
	void SetResourceByName(std::string ResourceName, CommonShaderResource* InResource)
	{
		for (int i = 0; i < CommonInfo.ShaderResourceSlots.size(); i++)
		{
			if (CommonInfo.ShaderResourceSlots[i].Name == ResourceName)
			{
				InternelSetResource(i, InResource);
			}
		}
	}
	//ͨ���󶨺��趨��Դ
	void SetResourceByBindingIndex(ShaderResourceType Type, unsigned int BindingIndex, CommonShaderResource* InResource)
	{
		for (int i = 0; i < CommonInfo.ShaderResourceSlots.size(); i++)
		{
			if ((CommonInfo.ShaderResourceSlots[i].Type == Type) &&
				(CommonInfo.ShaderResourceSlots[i].BindingIndex == BindingIndex))
			{
				InternelSetResource(i, InResource);
			}
		}
	}
};


//D3D12��ͼ�ι�������
class GraphicsPipeline : public CommonPipelineStateObject
{
public:
	GraphicsPipeline(GraphicsPipelineInfo Info) :CommonPipelineStateObject(Info) {}

	//��������Ӧ�ĸ���Դ
	std::vector<unsigned int> RootParameterResourceIndex;

	//��ǩ��
	ComPtr<ID3D12RootSignature> RootSignature;
	//D3D12��ͼ�ι��߶���
	ComPtr<ID3D12PipelineState> PipelineState;
	//�Ӵ�
	  D3D12_VIEWPORT Viewport;
	//�ü�����
	CD3DX12_RECT ScissorRect;
};

class PsoContainer
{
public:
	PsoContainer(Device* device, ComPtr<ID3D12RootSignature> rootSign);
	~PsoContainer();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  GetOpaquePsoDesc();
	D3D12_DEPTH_STENCIL_DESC  GetStencilDefault();
	D3D12_RENDER_TARGET_BLEND_DESC  GetTransparencyBlendDefault();
	void AddPsoContainer(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, RenderLayer index);
	void AddComputePsoContainer(D3D12_COMPUTE_PIPELINE_STATE_DESC desc, RenderLayer index);

	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> GetPsoMap() { return mPsos; }
	ID3D12PipelineState* GetPsoByRenderLayer(RenderLayer index)
	{
		if (mPsos.find(index) != mPsos.end()) 
		{
			return mPsos[index].Get();
		}
		return NULL;
	}
	void BuildShadersAndInputLayout();
	D3D12_SHADER_BYTECODE SetShader(std::string name);
	D3D12_INPUT_LAYOUT_DESC GetVerInputLayout();
	D3D12_INPUT_LAYOUT_DESC GetGeoInputLayout();
private:
	Device* mDevice;
	ComPtr<ID3D12RootSignature> mRootSign;

	//����ӳ�����
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> mPsos;

	std::unique_ptr<ShaderCompile> mShaders;

	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
