#pragma once
#include"RenderItem.h"
#include"../Utility/d3dx12.h"
#include<map>
class ShaderCompile;


//着色器资源的种类
enum ShaderResourceType
{
	UniformBuffer,
	Texture,
	Sampler,
};

//顶点输入属性的种类，对照于HLSL的语义
enum VertexInputAttributeType
{
	VIA_POSITION,	//顶点位置
	VIA_NORMAL,		//顶点法线
	VIA_TANGENT,	//顶点切线
	VIA_COLOR,		//顶点颜色
	VIA_TEXCOORD,	//贴图UV坐标
};

//代表一个着色器资源，比如UniformBuffer，或者纹理
class CommonShaderResource
{
};

//一个着色器资源的槽位信息
struct ShaderResourceSlotInfo
{
	ShaderResourceType Type;	//资源种类
	std::string Name;				//资源名字
	unsigned int BindingIndex;		//绑定的序号
	ShaderResourceSlotInfo(ShaderResourceType InType, std::string InName, unsigned int InBindingIndex)
	{
		Type = InType;
		Name = InName;
		BindingIndex = InBindingIndex;
	}
};

//描述一个图形管线的信息
struct GraphicsPipelineInfo
{
	std::string VertexShaderFile;	//顶点着色器的文件名
	std::string PixelShaderFile;	//像素着色器的文件名
	std::vector<VertexInputAttributeType> VertexInputAttributeTypes;//顶点输入属性的种类
	std::vector<ShaderResourceSlotInfo> ShaderResourceSlots;	//着色器资源的槽位，即描述符布局
};

//描述一个顶点属性，基本上是顶点属性的种类加上所占float的个数
struct VertexInputAttributeDescription
{
	VertexInputAttributeType Type;	//种类
	unsigned int Float32Cout;		//这个属性有多少个Float
	VertexInputAttributeDescription(VertexInputAttributeType InType)
	{
		Type = InType;

		//在此依照种类对Float32Cout做自动的设置：
		switch (Type)
		{
		case VIA_POSITION:
			Float32Cout = 3;	//XYZ三个方向
			break;
		case VIA_NORMAL:
			Float32Cout = 3;	//XYZ三个方向
			break;
		case VIA_TANGENT:
			Float32Cout = 3;	//XYZ三个方向
			break;
		case VIA_COLOR:
			Float32Cout = 4;	//RGBA四个通道
			break;
		case VIA_TEXCOORD:
			Float32Cout = 2;	//UV两个方向
			break;
		default:
			break;
		}
	}
};

//顶点属性的存取器
class VertexAttributeAccessor
{
	//顶点属性数据的指针：
	std::vector<float>* pVertexAttributeData;
	//每个顶点有多少个Float
	int VertexFloatAmount = 0;
	//每种属性对应的Float偏移
	std::map<VertexInputAttributeType, int> AttributeTypeOffset;
public:
	VertexAttributeAccessor(const std::vector<VertexInputAttributeDescription>& Layout
		, std::vector<float>* InVertexAttributeData)
	{
		pVertexAttributeData = InVertexAttributeData;

		for (int a = 0; a < Layout.size(); a++)
		{
			AttributeTypeOffset[Layout[a].Type] = VertexFloatAmount;//记录偏移
			VertexFloatAmount += Layout[a].Float32Cout;				//增加数目
		}
	}

	//设置属性
	void Set(int VertexIndex,					//顶点索引
		VertexInputAttributeType AttributeType,	//属性种类
		int direction,							//分量
		float value								//值
	)
	{
		(*pVertexAttributeData)[VertexIndex * VertexFloatAmount + AttributeTypeOffset[AttributeType] + direction] = value;
	}

	//获得属性
	float Get(int VertexIndex,					//顶点索引
		VertexInputAttributeType AttributeType,	//属性种类
		int direction							//分量
	)
	{
		return (*pVertexAttributeData)[VertexIndex * VertexFloatAmount + AttributeTypeOffset[AttributeType] + direction];
	}
};

//公共的管线状态对象，不同的图形API应对其有不同的继承并加入其需要的成员
class CommonPipelineStateObject
{
protected:
	//创建时提供的公共信息
	GraphicsPipelineInfo CommonInfo;

public:
	//顶点输入布局
	std::vector<VertexInputAttributeDescription> VertexInputAttributes;

	//当前的资源，对应于ShaderResourceSlots
	std::vector<CommonShaderResource*> CurrentShaderResources;


	//构造函数
	CommonPipelineStateObject(GraphicsPipelineInfo Info)
	{
		CommonInfo = Info;

		//自动设置顶点输入属性的描述
		for (int i = 0; i < Info.VertexInputAttributeTypes.size(); i++)
			VertexInputAttributes.push_back(VertexInputAttributeDescription(Info.VertexInputAttributeTypes[i]));

		//当前的资源初始为空
		for (int i = 0; i < Info.ShaderResourceSlots.size(); i++)
			CurrentShaderResources.push_back(nullptr);
	}

	const GraphicsPipelineInfo* GetCommonInfo() { return &CommonInfo; }

protected://内部调用

	//设定资源（子类应该继承它）
	//对于传统图形API，是直接将资源绑定到渲染管线上了
	//对于先进图形API，只是告诉管线其所需的资源是什么，真正的绑定需要在之后使用【命令】
	virtual void InternelSetResource(unsigned int ResourceIndex, CommonShaderResource* InResource)
	{
		CurrentShaderResources[ResourceIndex] = InResource;
	}

public://外部接口

	//通过名字设定资源
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
	//通过绑定号设定资源
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


//D3D12的图形管线数据
class GraphicsPipeline : public CommonPipelineStateObject
{
public:
	GraphicsPipeline(GraphicsPipelineInfo Info) :CommonPipelineStateObject(Info) {}

	//根参数对应哪个资源
	std::vector<unsigned int> RootParameterResourceIndex;

	//根签名
	ComPtr<ID3D12RootSignature> RootSignature;
	//D3D12的图形管线对象
	ComPtr<ID3D12PipelineState> PipelineState;
	//视窗
	  D3D12_VIEWPORT Viewport;
	//裁剪矩形
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

	//无序映射管理
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> mPsos;

	std::unique_ptr<ShaderCompile> mShaders;

	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
