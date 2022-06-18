#include"DefaultScene.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/FrameResource.h"

const int gNumFrameResources = 3;

DefaultScene::DefaultScene(HINSTANCE hInstance)
	: D3DScene(hInstance)
{
}

DefaultScene::~DefaultScene()
{
	if (m_Device->GetDevice() != nullptr)
		FlushCommandQueue();
}

bool DefaultScene::Initialize()
{
	if (!D3DScene::Initialize())
		return false;

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	m_SunLight.SetLight(1.25f * XM_PI, XM_PIDIV4, 1.0f, { 1.0f, 1.0f, 0.9f }, 0);

	BuildRootSignature();
	mPsoContainer = std::make_unique<PsoContainer>(m_Device.get(), mRootSignature);
	BuildGeometrys();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void DefaultScene::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPsoContainer->GetPsoByRenderLayer(RenderLayer::Opaque)));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));

	//用雾效果颜色填充场景
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, rvalue_to_lvalue(CurrentBackBufferView()), true, rvalue_to_lvalue(DepthStencilView()));

	m_ItemManager->SetDescriptorHeaps(mCommandList.Get());

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	////设置根描述符
	//int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
	//auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	////根参数的起始索引
	//mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	auto passCB = mCurrFrameResource->PassCB->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	auto matBuffer = mCurrFrameResource->MaterialBuffer->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	m_ItemManager->SetRootDescriptorTable(mCommandList.Get());

	for (auto& [k, v] : mPsoContainer->GetPsoMap())
	{
		mCommandList->SetPipelineState(v.Get());
		DrawRenderItems(mCommandList.Get(), k);
	}

	mCommandList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void DefaultScene::OnResize()
{
	D3DScene::OnResize();

	m_Camera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void DefaultScene::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	//UpdateLight(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	AnimateMaterials(gt);
	m_ItemManager->UpdateCBs(mCurrFrameResource);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);
}

#pragma region update具体实现
void DefaultScene::AnimateMaterials(const GameTimer& gt)
{
	auto waterMat = m_ItemManager->GetMaterial("water");

	float tu = waterMat->GetMatTransformValue(0, 3);
	float tv = waterMat->GetMatTransformValue(1, 3);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->SetMatTransformValue(0, 3, tu);
	waterMat->SetMatTransformValue(1, 3, tv);
	waterMat->SetNumFramesDirty(gNumFrameResources);

	auto boxMat = m_ItemManager->GetMaterial("flare");
	boxMat->SetMatTransform(DirectX::XMMatrixRotationZ(gt.TotalTime()));
	boxMat->SetNumFramesDirty(gNumFrameResources);
}

void DefaultScene::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = m_Camera.GetView();
	XMMATRIX proj = m_Camera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(rvalue_to_lvalue(XMMatrixDeterminant(view)), view);
	XMMATRIX invProj = XMMatrixInverse(rvalue_to_lvalue(XMMatrixDeterminant(proj)), proj);
	XMMATRIX invViewProj = XMMatrixInverse(rvalue_to_lvalue(XMMatrixDeterminant(viewProj)), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = m_Camera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	XMVECTOR lightDir = -m_SunLight.GetLightDir();
	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	mMainPassCB.Lights[0].Strength = m_SunLight.GetLightStrength();

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void DefaultScene::UpdateWaves(const GameTimer& gt)
{
	// 0.25生成随机波
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, m_Waves->RowCount() - 5);
		int j = MathHelper::Rand(4, m_Waves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		m_Waves->Disturb(i, j, r);
	}

	m_Waves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < m_Waves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = m_Waves->Position(i);
		v.Normal = m_Waves->Normal(i);

		v.TexC.x = 0.5f + v.Pos.x / m_Waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / m_Waves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	m_WavesRitem->Geo->SetVertexBufferGPU(currWavesVB->GetResource());
}

void DefaultScene::UpdateLight(const GameTimer& gt)
{
	m_SunLight.UpdateTheta(0.5f * gt.DeltaTime());

	//光强随时间 强弱强
	//mSinFactor += gt.DeltaTime();
	//if (mSinFactor >= 3.14)
	//	mSinFactor = 0;
	//mLightIntensity = sinf(mSinFactor);
}

void  DefaultScene::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void  DefaultScene::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void  DefaultScene::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		m_Camera.Pitch(dy);
		m_Camera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void DefaultScene::OnKeyboardInput(const GameTimer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;

	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		m_SunLight.UpdateTheta(-1.0f * dt);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		m_SunLight.UpdateTheta(1.0f * dt);

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		m_SunLight.UpdatePhi(-1.0f * dt);

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		m_SunLight.UpdatePhi(1.0f * dt);

	if (GetAsyncKeyState('W') & 0x8000)
		m_Camera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_Camera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_Camera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_Camera.Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		m_Camera.RotateY(-1.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		m_Camera.RotateY(1.0f * dt);

	m_Camera.UpdateViewMatrix();
}
#pragma endregion

#pragma region 几何体构造
void DefaultScene::WavesGeometry()
{
	std::vector<std::uint16_t> indices(3 * m_Waves->TriangleCount()); // 3 indices per face
	assert(m_Waves->VertexCount() < 0x0000ffff);

	int m = m_Waves->RowCount();
	int n = m_Waves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = m_Waves->VertexCount() * sizeof(Vertex);

	m_ItemManager->GetMeshManager()->CreateMeshVertexUpload("waterGeo", vbByteSize, indices);
	m_ItemManager->GetMeshManager()->CreateSubMesh("waterGeo", "grid", (UINT)indices.size(), 0,0);
}
void DefaultScene::LandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = grid.GetIndices16();

	m_ItemManager->GetMeshManager()->CreateMesh("landGeo", vertices, indices);
	m_ItemManager->GetMeshManager()->CreateSubMesh("landGeo", "grid", (UINT)indices.size(), 0, 0);
}
void DefaultScene::ShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size(); 
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	m_ItemManager->GetMeshManager()->CreateMesh("shapeGeo", vertices, indices);
	m_ItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "box", (UINT)box.Indices32.size(), boxIndexOffset, boxVertexOffset);
	m_ItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "grid", (UINT)grid.Indices32.size(), gridIndexOffset, gridVertexOffset);
	m_ItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "sphere", (UINT)sphere.Indices32.size(), sphereIndexOffset, sphereVertexOffset);
	m_ItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "cylinder", (UINT)cylinder.Indices32.size(), cylinderIndexOffset, cylinderVertexOffset);
}
void DefaultScene::BuildSkullGeometry()
{
	std::ifstream fin("Resources/Models/car.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::uint16_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	m_ItemManager->GetMeshManager()->CreateMesh("skullGeo", vertices, indices);
	m_ItemManager->GetMeshManager()->CreateSubMesh("skullGeo", "skull", (UINT)indices.size(), 0, 0);
}
void DefaultScene::BuildGeometrys()
{
	WavesGeometry();
	LandGeometry();
	ShapeGeometry();
	BuildSkullGeometry();
	m_ItemManager->GetMeshManager()->LoadMesh("Resources/Models/cow.obj", "loadGeo", "cow");
}
#pragma endregion

//shader中采样模式只有几种，把他们列举出来
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6>  DefaultScene::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

//将资源绑定到对应的缓冲区
void DefaultScene::BuildRootSignature()
{
	//11个srv
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 0,0);

	// 根参数表
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	//space0 b0
	slotRootParameter[0].InitAsConstantBufferView(0);
	//space0 b1
	slotRootParameter[1].InitAsConstantBufferView(1);
	//space1 t0
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	//// Create a single descriptor table of CBVs.
	//CD3DX12_DESCRIPTOR_RANGE cbvTable;
	////类型,描述符数量,寄存器编号
	//cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	//CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	//cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	auto staticSamplers = GetStaticSamplers();
	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(m_Device->GetDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

//cpu gpu之间通信的围栏点
void DefaultScene::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(m_Device.get(),
			1, (UINT)m_ItemManager->ItemsSize(), (UINT)m_ItemManager->MaterialsSize(), m_Waves->VertexCount()));
	}
}

//搭建渲染管线
void DefaultScene::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  opaquePsoDesc = mPsoContainer->GetOpaquePsoDesc();
	mPsoContainer->AddPsoContainer(opaquePsoDesc,RenderLayer::Opaque);

	//纹理旋转融合指定单独shader
	D3D12_GRAPHICS_PIPELINE_STATE_DESC texRotatePsoDesc= mPsoContainer->GetOpaquePsoDesc();
	texRotatePsoDesc.VS = mPsoContainer->SetShader("rotateVS");
	mPsoContainer->AddPsoContainer(texRotatePsoDesc,RenderLayer::TexRotate);

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC  NoTextureDesc= opaquePsoDesc;
	//NoTextureDesc.VS = m_Shaders->GetShaderBYTE("NoTextureVS");
	//NoTextureDesc.PS = m_Shaders->GetShaderBYTE("NoTexturePS");
	//ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&NoTextureDesc, IID_PPV_ARGS(&m_PSOs[RenderLayer::NoTexture])));

	//透明海水混合
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = mPsoContainer->GetOpaquePsoDesc();

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;//混合模式
	transparencyBlendDesc.LogicOpEnable = false;//逻辑模式 ，和混合模式只能启用一个
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

	mPsoContainer->AddPsoContainer(transparentPsoDesc,RenderLayer::Transparent);

	//雾效果和alpha剔除
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = mPsoContainer->GetOpaquePsoDesc();
	alphaTestedPsoDesc.PS = mPsoContainer->SetShader("alphaTestedPS");
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	mPsoContainer->AddPsoContainer(alphaTestedPsoDesc,RenderLayer::AlphaTested);

	//线框模式
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = mPsoContainer->GetOpaquePsoDesc();
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	mPsoContainer->AddPsoContainer(opaqueWireframePsoDesc,RenderLayer::Wireframe);
}

//定义材料属性
void DefaultScene::BuildMaterials()
{
	m_ItemManager->BuildMaterial("grass", 0, XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0);
	m_ItemManager->BuildMaterial("water", 1, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0);
	m_ItemManager->BuildMaterial("crate",  2, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("bricks",  3, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.02f, 0.02f, 0.02f), 0.1f);
	m_ItemManager->BuildMaterial("stone",  4, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	m_ItemManager->BuildMaterial("tile",  5, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.02f, 0.02f, 0.02f), 0.3f);
	m_ItemManager->BuildMaterial("wirefence",  6, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("flare",  7, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("flarealpha",  8, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("cow", 9, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05), 0.25f);
	m_ItemManager->BuildMaterial("rock",10, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05), 0.25f);
}

void DefaultScene::BuildRenderItems()
{
	//水
	m_ItemManager->BuildRenderItem("water", RenderLayer::Opaque, "waterGeo", "grid", "water",
		PositionMatrix(1.0f, 1.0f, 1.0f,0.0f, 0.0f, 0.0f),
		PositionMatrix(5.0f, 5.0f, 1.0f,0.0f, 0.0f, 0.0f));
	m_WavesRitem= m_ItemManager->GetRenderItem("water");
	//草地
	m_ItemManager->BuildRenderItem("grid", RenderLayer::Opaque,  "landGeo", "grid", "grass",
		PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		PositionMatrix(5.0f, 5.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	m_ItemManager->BuildRenderItem("grass", RenderLayer::Opaque, "shapeGeo", "sphere", "grass",
		PositionMatrix(3.0f, 3.0f, 3.0f,3.0f, 5.0f, -9.0f),
		PositionMatrix(1.0f, 1.0f, 1.0f,0.0f, 0.0f, 0.0f));
	m_ItemManager->BuildRenderItem("box", RenderLayer::TexRotate, "shapeGeo","box", "flare",
		PositionMatrix(3.0f, 3.0f, 3.0f, 3.0f, 9.0f, -9.0f),
		PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	m_ItemManager->BuildRenderItem("box2", RenderLayer::Opaque, "shapeGeo", "sphere", "stone",
		PositionMatrix(3.0f, 3.0f, 3.0f, 7.0f, 2.0f, -9.0f),
		PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	m_ItemManager->BuildRenderItem("box3", RenderLayer::AlphaTested, "shapeGeo", "box", "wirefence",
		PositionMatrix(3.0f, 3.0f, 3.0f,7.0f, 1.0f, -14.0f),
		PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	m_ItemManager->BuildRenderItem("cow", RenderLayer::Opaque,"loadGeo", "cow", "cow",
		PositionMatrix(13.0f, 13.0f, 13.0f, 7.0f, 17.0f, -14.0f),
		PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	//m_ItemManager->BuildRenderItem("rock", RenderLayer::Opaque, "rockGeo", "rock", "rock",
	//	PositionMatrix(13.0f, 13.0f, 13.0f, 7.0f, 17.0f, -14.0f),
	//	PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
	
	//m_ItemManager->BuildRenderItem("cow1", RenderLayer::Opaque, "skullGeo", "skull", "stone",
	//	PositionMatrix(13.0f, 13.0f, 13.0f, 7.0f, 17.0f, -14.0f),
	//	PositionMatrix(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f));
}

void DefaultScene::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->GetResource();

	m_ItemManager->DrawRenderItems(objCBByteSize, objectCB->GetGPUVirtualAddress(),cmdList, name);
}

//大小x旋转 无大小改变为1，无旋转改变为0
FXMMATRIX DefaultScene::PositionMatrix(float scaleX,float scaleY,float scaleZ, float  translateX,float translateY,float translateZ)
{
	return  XMMatrixScaling(scaleX, scaleY, scaleZ) * XMMatrixTranslation(translateX, translateY, translateZ);
}

float DefaultScene::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 DefaultScene::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}