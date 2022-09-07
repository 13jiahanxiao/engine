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

	m_SunLight.SetLight(1.25f * XM_PI, XM_PIDIV4, 1.0f, { 1.0f, 1.0f, 0.9f }, 0);

	LoadAssetManager = std::make_unique<REngine::LoadAsset>();

	mTextureHeap = std::make_unique<DescriptorHeap>(m_Device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,128, true);
	//创建描述符
	LoadAssetManager->CreateTexture(m_Device.get(), mCommandList.Get());
	LoadAssetManager->BuildTextureHeap(mTextureHeap.get());

	mItemManager->CreateMaterial(LoadAssetManager.get());
	mRootsignature->Init(LoadAssetManager->GetTextureNum());

	mPostProcess->SetDescriptorHeapAndOffset(mTextureHeap.get());

	mPostProcess->InitBlurFilter(mClientWidth/2, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

	mPsoContainer = std::make_unique<PsoContainer>(m_Device.get(), mRootsignature->GetRootSign());
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

	////用雾效果颜色填充场景
	//mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, rvalue_to_lvalue(CurrentBackBufferView()), true, rvalue_to_lvalue(DepthStencilView()));

	//指针数组 每个指针指向ID3D12DescriptorHeap
	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureHeap->GetHeap() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootsignature->GetRootSign().Get());

	////设置根描述符
	//int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
	//auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	////根参数的起始索引
	//mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawItems();

	mPostProcess->Execute(mCommandList.Get(), mRootsignature->GetPostProcessRootSign().Get(), mPsoContainer->GetPsoByRenderLayer(RenderLayer::HorzBlur),
	mPsoContainer->GetPsoByRenderLayer(RenderLayer::VertBlur), CurrentBackBuffer(), 20);

	// Prepare to copy blurred output to the back buffer.
	mCommandList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST)));

	mCommandList->CopyResource(CurrentBackBuffer(), mPostProcess->BlurFilterOutput());

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

	//CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mTextureHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart());
	//skyTexDescriptor.Offset(13, mCbvSrvUavDescriptorSize);
	//mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	mCommandList->SetGraphicsRootDescriptorTable(4, mTextureHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart());

	DrawItemByPsoLayer(RenderLayer::Opaque);
	//DrawItemByPsoLayer(RenderLayer::Wireframe);
	//DrawItemByPsoLayer(RenderLayer::AlphaTested);
	//DrawItemByPsoLayer(RenderLayer::TexRotate);
	//DrawItemByPsoLayer(RenderLayer::BillBoardTree);
	//mCommandList->OMSetStencilRef(1);
	//DrawItemByPsoLayer(RenderLayer::Mirror);
	////2个pass要在frame体现出来
	//UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	//mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	//DrawItemByPsoLayer(RenderLayer::Reflection);
	//DrawItemByPsoLayer(RenderLayer::AlphaTestedAndRefection);
	//mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
	//mCommandList->OMSetStencilRef(0);
	////做混合
	//DrawItemByPsoLayer(RenderLayer::Transparent);
	//DrawItemByPsoLayer(RenderLayer::Sky);


}

void DefaultScene::OnResize()
{
	D3DScene::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	//后处理
	mPostProcess->OnResize();
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
	//AnimateMaterials(gt);
	mItemManager->UpdateCBs(mCurrFrameResource);
	UpdateMainPassCB(gt);
	UpdateReflectPassCB(gt);
}

#pragma region update具体实现
void DefaultScene::AnimateMaterials(const GameTimer& gt)
{
	auto waterMat = mItemManager->GetMaterial("water");

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

	auto boxMat = mItemManager->GetMaterial("flare");
	boxMat->SetMatTransform(DirectX::XMMatrixRotationZ(gt.TotalTime()));
	boxMat->SetNumFramesDirty(gNumFrameResources);
}

void DefaultScene::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

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
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
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

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
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
		mCamera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.RotateY(-1.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.RotateY(1.0f * dt);

	mCamera.UpdateViewMatrix();
}
#pragma endregion

#pragma region 几何体构造
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

	mItemManager->GetMeshManager()->CreateMesh("shapeGeo", vertices, indices);
	mItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "box", (UINT)box.Indices32.size(), boxIndexOffset, boxVertexOffset);
	mItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "grid", (UINT)grid.Indices32.size(), gridIndexOffset, gridVertexOffset);
	mItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "sphere", (UINT)sphere.Indices32.size(), sphereIndexOffset, sphereVertexOffset);
	mItemManager->GetMeshManager()->CreateSubMesh("shapeGeo", "cylinder", (UINT)cylinder.Indices32.size(), cylinderIndexOffset, cylinderVertexOffset);
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

	mItemManager->GetMeshManager()->CreateGeoMesh("treeSpritesGeo", vertices, indices);
	mItemManager->GetMeshManager()->CreateSubMesh("treeSpritesGeo", "points", (UINT)indices.size(), 0, 0);
}

void DefaultScene::BuildGeometrys()
{
	ShapeGeometry();
	BillTreeGeometry();
	mItemManager->GetMeshManager()->LoadMesh(LoadAssetManager.get());
}
#pragma endregion

//cpu gpu之间通信的围栏点
void DefaultScene::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(m_Device.get(),
			2, (UINT)mItemManager->ItemsSize(), (UINT)mItemManager->GetMaterilalsNum()));
	}
}

//搭建渲染管线
void DefaultScene::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  opaquePsoDesc = mPsoContainer->GetOpaquePsoDesc();
	opaquePsoDesc.RasterizerState.CullMode =D3D12_CULL_MODE_NONE;
	mPsoContainer->AddPsoContainer(opaquePsoDesc,RenderLayer::Opaque);

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = mPsoContainer->GetOpaquePsoDesc();
	//skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	//skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//skyPsoDesc.VS = mPsoContainer->SetShader("skyVS");
	//skyPsoDesc.PS = mPsoContainer->SetShader("skyPS");
	//mPsoContainer->AddPsoContainer(skyPsoDesc, RenderLayer::Sky);

	D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
	horzBlurPSO.pRootSignature = mRootsignature->GetPostProcessRootSign().Get();
	horzBlurPSO.CS = mPsoContainer->SetShader("horzBlurCS");
	horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	mPsoContainer->AddComputePsoContainer(horzBlurPSO, RenderLayer::HorzBlur);

	D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
	vertBlurPSO.pRootSignature = mRootsignature->GetPostProcessRootSign().Get();
	vertBlurPSO.CS = mPsoContainer->SetShader("vertBlurCS");
	vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	mPsoContainer->AddComputePsoContainer(vertBlurPSO, RenderLayer::VertBlur);
}

void DefaultScene::BuildRenderItems()
{
    mItemManager->LoadRenderItemFromJson();

	//mItemManager->BuildRenderItem("skyBox", RenderLayer::Sky, "shapeGeo", "sphere", "skyBox",
	//	MathHelper::PositionMatrix(5.0f, 5.0f, 5.0f), MathHelper::PositionMatrix());
}

void DefaultScene::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->GetResource();

	mItemManager->DrawRenderItems(objCBByteSize, objectCB->GetGPUVirtualAddress(),cmdList, name);
}

float DefaultScene::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.01f * x) + x * cosf(0.02f * z));
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

void DefaultScene::DrawItemByPsoLayer(RenderLayer renderLayer)
{
	mCommandList->SetPipelineState(mPsoContainer->GetPsoByRenderLayer(renderLayer));
	DrawRenderItems(mCommandList.Get(), renderLayer);
}