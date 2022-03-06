struct VertexIn
{
	float3 PosL    : POSITION;
};

struct VertexOut
{
	float3 PosL    : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosL = vin.Vertex;
	return vout;
}

struct PatchTess
{
	float edgeTess[4] : SV_TessFactor; //4条边的细分因子
	float insideTess[2] : SV_InsideTessFactor; //内部的细分因子（u和v两个方向）
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, //输入的面片控制点
	uint patchID : SV_PrimitiveID) //面片图元ID
{
	PatchTess pt;

	//计算出面片的中点
	float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL) * 0.25f;
	//将中心点从模型空间转到世界空间下
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
	//计算摄像机和面片的距离
	float d = distance(centerW, gEyePosW);

	//LOD的变化区间（最大和最小区间）
	float d0 = 20.0f;
	float d1 = 100.0f;
	//随着距离变化，计算细分因子,注意先减后除的括号！！！！！
	float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	//赋值所有边和内部的细分因子
	pt.edgeTess[0] = tess;
	pt.edgeTess[1] = tess;
	pt.edgeTess[2] = tess;
	pt.edgeTess[3] = tess;
	pt.insideTess[0] = tess;
	pt.insideTess[1] = tess;

	return pt;
}


struct HullOut
{
	float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
	uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	hout.PosL = p[i].PosL;

	return hout;
}

struct DomainOut
{
	float4 PosH : SV_POSITION;
};

[domain("quad")]
DomainOut DS(PatchTess pt, //细分因子
	float2 uv : SV_DomainLocation, //细分后顶点UV（位置UV，非纹理UV）
	const OutputPatch<HullOut, 4> patch)//patch的4个控制点
{
	DomainOut dout;

	//使用双线性插值(先算横向，再算纵向)算出控制点坐标
	float3 v1 = lerp(patch[0].PosL, patch[1].PosL, uv.x);
	float3 v2 = lerp(patch[2].PosL, patch[3].PosL, uv.x);
	float3 v = lerp(v1, v2, uv.y);

	//计算控制点高度（Y值）
	v.y = 0.3f * (v.z * sin(v.x) + v.x * cos(v.z));

	//将控制点坐标转换到裁剪空间
	float4 posW = mul(float4(v, 1.0f), gWorld);
	dout.PosH = mul(posW, gViewProj);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 0.0f);
}