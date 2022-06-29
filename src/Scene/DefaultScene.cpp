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

	DrawItems();

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

void DefaultScene::DrawItems() 
{
	auto passCB = mCurrFrameResource->PassCB->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	auto matBuffer = mCurrFrameResource->MaterialBuffer->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	m_ItemManager->SetRootDescriptorTable(mCommandList.Get());

	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::Opaque));
	DrawRenderItems(mCommandList.Get(), RenderLayer::Opaque);

	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::AlphaTested));
	DrawRenderItems(mCommandList.Get(), RenderLayer::AlphaTested);

	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::TexRotate));
	DrawRenderItems(mCommandList.Get(), RenderLayer::TexRotate);

	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::BillBoardTree));
	DrawRenderItems(mCommandList.Get(), RenderLayer::BillBoardTree);

	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::Mirror));
	DrawRenderItems(mCommandList.Get(), RenderLayer::Mirror);

	//2个pass要在frame体现出来
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::Reflection));
	DrawRenderItems(mCommandList.Get(), RenderLayer::Reflection);

	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::AlphaTestedAndRefection));
	DrawRenderItems(mCommandList.Get(), RenderLayer::AlphaTestedAndRefection);

	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
	mCommandList->OMSetStencilRef(0);

	//做混合
	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(RenderLayer::Transparent));
	DrawRenderItems(mCommandList.Get(), RenderLayer::Transparent);
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
	UpdateReflectPassCB(gt);
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

void DefaultScene::UpdateReflectPassCB(const GameTimer& gt)
{
	mReflectPassCB = mMainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(5.0f, 0.0f, 0.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
	XMStoreFloat3(&mReflectPassCB.Lights[0].Direction, reflectedLightDir);

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectPassCB);
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
	GeometryGenerator::MeshData mirror = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);

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
void DefaultScene::BillTreeGeometry()
{
	static const int treeCount = 16;
	std::vector<GeoVertex>vertices;
	std::vector<std::uint16_t>indices;
	vertices.resize(treeCount);
	indices.resize(treeCount);
	for (UINT i = 0; i < treeCount; ++i) 
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
		indices[i] = i;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(GeoVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_ItemManager->GetMeshManager()->CreateGeoMesh("treeSpritesGeo", vertices, indices);
	m_ItemManager->GetMeshManager()->CreateSubMesh("treeSpritesGeo", "points", (UINT)indices.size(), 0, 0);
}

void DefaultScene::BuildGeometrys()
{
	WavesGeometry();
	LandGeometry();
	ShapeGeometry();
	BuildSkullGeometry();
	BillTreeGeometry();
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
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 13, 0,0);

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
			2, (UINT)m_ItemManager->ItemsSize(), (UINT)m_ItemManager->GetMaterilalsNum(), m_Waves->VertexCount()));
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

	//mirror
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = mPsoContainer->GetOpaquePsoDesc();
	markMirrorsPsoDesc.BlendState = mirrorBlendState;
	markMirrorsPsoDesc.DepthStencilState = mPsoContainer->GetStencilDefault();
	mPsoContainer->AddPsoContainer(markMirrorsPsoDesc, RenderLayer::Mirror);

	//映射物体
	D3D12_DEPTH_STENCIL_DESC reflectionsDSS = mPsoContainer->GetStencilDefault();
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = mPsoContainer->GetOpaquePsoDesc();
	drawReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
	drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
	mPsoContainer->AddPsoContainer(drawReflectionsPsoDesc, RenderLayer::Reflection);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphatextAndReflect = drawReflectionsPsoDesc;
	alphatextAndReflect.PS = mPsoContainer->SetShader("alphaTestedPS");
	alphatextAndReflect.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	mPsoContainer->AddPsoContainer(alphaTestedPsoDesc, RenderLayer::AlphaTestedAndRefection);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = mPsoContainer->GetOpaquePsoDesc();
	treeSpritePsoDesc.InputLayout = mPsoContainer->GetGeoInputLayout();
	treeSpritePsoDesc.VS = mPsoContainer->SetShader("treeSpriteVS");
	treeSpritePsoDesc.GS = mPsoContainer->SetShader("treeSpriteGS");
	treeSpritePsoDesc.PS = mPsoContainer->SetShader("treeSpritePS");
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	mPsoContainer->AddPsoContainer(treeSpritePsoDesc, RenderLayer::BillBoardTree);


}

void DefaultScene::BuildRenderItems()
{
	//水
	m_ItemManager->BuildRenderItem("water", RenderLayer::Transparent, "waterGeo", "grid", "water",
		PositionMatrix(),PositionMatrix(5.0f, 5.0f, 1.0f));
	m_WavesRitem= m_ItemManager->GetRenderItem("water");
	//草地
	m_ItemManager->BuildRenderItem("grid", RenderLayer::Opaque,  "landGeo", "grid", "grass",
		PositionMatrix(),	PositionMatrix(5.0f, 5.0f, 1.0f));
	m_ItemManager->BuildRenderItem("grass", RenderLayer::Opaque, "shapeGeo", "sphere", "grass",
		PositionMatrix(3.0f, 3.0f, 3.0f,7.0f, 8.0f, 15.0f),PositionMatrix());
	m_ItemManager->BuildRenderItem("sphere", RenderLayer::Opaque, "shapeGeo", "sphere", "stone",
		PositionMatrix(3.0f, 3.0f, 3.0f, 7.0f, 8.0f, 5.0f),PositionMatrix());
	m_ItemManager->BuildRenderItem("cow", RenderLayer::Opaque, "loadGeo", "cow", "cow",
		PositionMatrix(4.0f, 4.0f, 4.0f, 11.0f,8.0f), PositionMatrix());
	//这里旋转针对材质
	m_ItemManager->BuildRenderItem("flareBox", RenderLayer::TexRotate, "shapeGeo", "box", "flare",
		PositionMatrix(3.0f, 3.0f, 3.0f, 10.0f, 8.0f, -5.0f), PositionMatrix());
	m_ItemManager->BuildRenderItem("wirefenceBox", RenderLayer::AlphaTested, "shapeGeo", "box", "wirefence",
		PositionMatrix(3.0f, 3.0f, 3.0f,10.0f, 8.0f, -10.0f),PositionMatrix());


	m_ItemManager->BuildRenderItem("mirror", RenderLayer::Mirror, "shapeGeo", "grid", "ice",
		PositionMatrix(1.0f, 1.0f, 1.0f, 5.0f, 8.0f, 0.0f,-0.5),PositionMatrix());

	m_ItemManager->BuildRenderItem("cowReflection", RenderLayer::Reflection, "loadGeo", "cow", "cow",
		PositionMatrix(4.0f, 4.0f, 4.0f, -1.0f, 8.0f), PositionMatrix());
	m_ItemManager->BuildRenderItem("grassReflection", RenderLayer::Reflection, "shapeGeo", "sphere", "grass",
		PositionMatrix(3.0f, 3.0f, 3.0f, 3.0f, 8.0f, 15.0f), PositionMatrix());
	m_ItemManager->BuildRenderItem("sphereReflection", RenderLayer::Reflection, "shapeGeo", "sphere", "stone",
		PositionMatrix(3.0f, 3.0f, 3.0f, 3.0f, 8.0f, 5.0f), PositionMatrix());
	m_ItemManager->BuildRenderItem("flareBoxR", RenderLayer::Reflection, "shapeGeo", "sphere", "flare",
		PositionMatrix(3.0f, 3.0f, 3.0f, 0.0f, 8.0f, -5.0f), PositionMatrix());
	m_ItemManager->BuildRenderItem("wirefenceBoxR", RenderLayer::AlphaTestedAndRefection, "shapeGeo", "box", "wirefence",
		PositionMatrix(3.0f, 3.0f, 3.0f, 0.0f, 8.0f, -10.0f), PositionMatrix());

	m_ItemManager->AddRenderItemInLayer("mirror", RenderLayer::Transparent);

	m_ItemManager->BuildRenderItem("treeSprites", RenderLayer::BillBoardTree, "treeSpritesGeo", "points", "treeTex",
		PositionMatrix(), PositionMatrix(), D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void DefaultScene::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->GetResource();

	m_ItemManager->DrawRenderItems(objCBByteSize, objectCB->GetGPUVirtualAddress(),cmdList, name);
}

//大小*旋转*平移 
FXMMATRIX DefaultScene::PositionMatrix(float scaleX,float scaleY,float scaleZ, 
	float  translateX,float translateY ,float translateZ ,
	float rotationZ)
{
	XMMATRIX Rotate = DirectX::XMMatrixRotationZ(rotationZ * MathHelper::Pi);
	XMMATRIX Scale = DirectX::XMMatrixScaling(scaleX, scaleY, scaleZ);
	XMMATRIX Offset = DirectX::XMMatrixTranslation(translateX, translateY, translateZ);
	XMMATRIX World = Scale*Rotate * Offset;
	return World;
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