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
	float edgeTess[4] : SV_TessFactor; //4���ߵ�ϸ������
	float insideTess[2] : SV_InsideTessFactor; //�ڲ���ϸ�����ӣ�u��v��������
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, //�������Ƭ���Ƶ�
	uint patchID : SV_PrimitiveID) //��ƬͼԪID
{
	PatchTess pt;

	//�������Ƭ���е�
	float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL) * 0.25f;
	//�����ĵ��ģ�Ϳռ�ת������ռ���
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
	//�������������Ƭ�ľ���
	float d = distance(centerW, gEyePosW);

	//LOD�ı仯���䣨������С���䣩
	float d0 = 20.0f;
	float d1 = 100.0f;
	//���ž���仯������ϸ������,ע���ȼ���������ţ���������
	float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	//��ֵ���бߺ��ڲ���ϸ������
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
DomainOut DS(PatchTess pt, //ϸ������
	float2 uv : SV_DomainLocation, //ϸ�ֺ󶥵�UV��λ��UV��������UV��
	const OutputPatch<HullOut, 4> patch)//patch��4�����Ƶ�
{
	DomainOut dout;

	//ʹ��˫���Բ�ֵ(���������������)������Ƶ�����
	float3 v1 = lerp(patch[0].PosL, patch[1].PosL, uv.x);
	float3 v2 = lerp(patch[2].PosL, patch[3].PosL, uv.x);
	float3 v = lerp(v1, v2, uv.y);

	//������Ƶ�߶ȣ�Yֵ��
	v.y = 0.3f * (v.z * sin(v.x) + v.x * cos(v.z));

	//�����Ƶ�����ת�����ü��ռ�
	float4 posW = mul(float4(v, 1.0f), gWorld);
	dout.PosH = mul(posW, gViewProj);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 0.0f);
}