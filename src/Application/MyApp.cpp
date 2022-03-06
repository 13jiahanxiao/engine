#include"MyApp.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/FrameResource.h"

const int gNumFrameResources = 3;
const int textureHeapNum = 9;
MyApp::MyApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

MyApp::~MyApp()
{
	if (m_Device->GetDevice() != nullptr)
		FlushCommandQueue();
}

bool MyApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList.Get()->Reset(mDirectCmdListAlloc.Get(), nullptr));

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	m_Camera.SetPosition(0.0f, 2.0f, -15.0f);

	m_SunLight.SetLight(1.25f * XM_PI, XM_PIDIV4, 1.0f, { 1.0f, 1.0f, 0.9f }, 0);

	m_ItemManager = std::make_unique<RenderItemManager>(textureHeapNum, m_Device.get());

	m_ItemManager->LoadTextureAndBuildTexureHeap(mCommandList.Get());

	m_Shaders = std::make_unique<ShaderCompile>();

	BuildRootSignature();
	BuildShadersAndInputLayout();
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

void MyApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}

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
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), RenderLayer::Opaque);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), RenderLayer::Transparent);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), RenderLayer::AlphaTested);

	mCommandList->SetPipelineState(mPSOs["texRotate"].Get());
	DrawRenderItems(mCommandList.Get(), RenderLayer::TexRotate);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, rvalue_to_lvalue(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void MyApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void MyApp::Update(const GameTimer& gt)
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
void MyApp::AnimateMaterials(const GameTimer& gt)
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

void MyApp::UpdateMainPassCB(const GameTimer& gt)
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

void MyApp::UpdateWaves(const GameTimer& gt)
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
	m_WavesRitem->Geo->VertexBufferGPU = currWavesVB->GetResource();
}

void MyApp::UpdateLight(const GameTimer& gt)
{
	m_SunLight.UpdateTheta(0.5f * gt.DeltaTime());

	//光强随时间 强弱强
	//mSinFactor += gt.DeltaTime();
	//if (mSinFactor >= 3.14)
	//	mSinFactor = 0;
	//mLightIntensity = sinf(mSinFactor);
}

void  MyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void  MyApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void  MyApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void MyApp::OnKeyboardInput(const GameTimer& gt)
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
void MyApp::WavesGeometry()
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
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device->GetDevice(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void MyApp::LandGeometry()
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

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device->GetDevice(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device->GetDevice(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void MyApp::ShapeGeometry()
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

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

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

	//总共顶点大小
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	//总共索引大小
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device->GetDevice(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device->GetDevice(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildGeometrys()
{
	WavesGeometry();
	LandGeometry();
	ShapeGeometry();
}
#pragma endregion

//shader中采样模式只有几种，把他们列举出来
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6>  MyApp::GetStaticSamplers()
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
void MyApp::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.  

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// 根参数表
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);
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

//绑定shader，并指定字段大小
void MyApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO rotate[] =
	{
		"ROT","1",NULL,NULL
	};

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG","1",NULL,NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG","1","ALPHA_TEST","1",NULL,NULL
	};

	m_Shaders->LoadShader("standardVS", L"Resources\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	m_Shaders->LoadShader("rotateVS", L"Resources\\Shaders\\Default.hlsl", rotate, "VS", "vs_5_0");
	m_Shaders->LoadShader("opaquePS", L"Resources\\Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
	m_Shaders->LoadShader("alphaTestedPS", L"Resources\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");

	m_Shaders->SetInputLayout(
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		}
	);
	
}

//cpu gpu之间通信的围栏点
void MyApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(m_Device.get(),
			1, (UINT)m_ItemManager->ItemsSize(), (UINT)m_ItemManager->MaterialsSize(), m_Waves->VertexCount()));
	}
}

//搭建渲染管线
void MyApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = m_Shaders->GetInputLayout();
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = m_Shaders->GetShaderBYTE("standardVS");
	opaquePsoDesc.PS = m_Shaders->GetShaderBYTE("opaquePS");
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//纹理旋转融合指定单独shader
	D3D12_GRAPHICS_PIPELINE_STATE_DESC texRotatePsoDesc = opaquePsoDesc;
	texRotatePsoDesc.VS = m_Shaders->GetShaderBYTE("rotateVS");
	ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&texRotatePsoDesc, IID_PPV_ARGS(&mPSOs["texRotate"])));

		//透明海水混合
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
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
	ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])))

		//雾效果和alpha剔除
		D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = m_Shaders->GetShaderBYTE("alphaTestedPS");
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//线框模式
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_Device->GetDevice()->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

//定义材料属性
void MyApp::BuildMaterials()
{
	m_ItemManager->BuildMaterial("grass", 0, 0, XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0);
	m_ItemManager->BuildMaterial("water", 1, 1, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0);
	m_ItemManager->BuildMaterial("crate", 2, 2, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("bricks", 3, 3, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.02f, 0.02f, 0.02f), 0.1f);
	m_ItemManager->BuildMaterial("stone", 4, 4, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	m_ItemManager->BuildMaterial("tile", 5, 5, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.02f, 0.02f, 0.02f), 0.3f);
	m_ItemManager->BuildMaterial("wirefence", 6, 6, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("flare", 7, 7, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
	m_ItemManager->BuildMaterial("flarealpha", 8, 8, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.25f);
}

void MyApp::BuildRenderItems()
{
	//水
	m_ItemManager->BuildRenderItem("water", RenderLayer::Opaque, 0, mGeometries["waterGeo"].get(), "grid", "water",
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
		PositionMatrix(DirectX::XMFLOAT3(5.0f, 5.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
	m_WavesRitem= m_ItemManager->GetRenderItem("water");
	//草地
	m_ItemManager->BuildRenderItem("grid", RenderLayer::Opaque, 1, mGeometries["landGeo"].get(), "grid", "grass",
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)),
		PositionMatrix(DirectX::XMFLOAT3(5.0f, 5.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
	m_ItemManager->BuildRenderItem("grass", RenderLayer::Opaque, 2, mGeometries["shapeGeo"].get(), "sphere", "grass",
		PositionMatrix(DirectX::XMFLOAT3(3.0f, 3.0f, 3.0f), DirectX::XMFLOAT3(3.0f, 5.0f, -9.0f)),
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
	m_ItemManager->BuildRenderItem("box", RenderLayer::TexRotate, 3, mGeometries["shapeGeo"].get(), "box", "flare",
		PositionMatrix(DirectX::XMFLOAT3(3.0f, 3.0f, 3.0f), DirectX::XMFLOAT3(3.0f, 9.0f, -9.0f)),
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
	m_ItemManager->BuildRenderItem("box2", RenderLayer::Opaque, 4, mGeometries["shapeGeo"].get(), "sphere", "stone",
		PositionMatrix(DirectX::XMFLOAT3(3.0f, 3.0f, 3.0f), DirectX::XMFLOAT3(7.0f, 2.0f, -9.0f)),
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
	m_ItemManager->BuildRenderItem("box3", RenderLayer::AlphaTested, 5, mGeometries["shapeGeo"].get(), "box", "wirefence",
		PositionMatrix(DirectX::XMFLOAT3(3.0f, 3.0f, 3.0f), DirectX::XMFLOAT3(7.0f, 1.0f, -14.0f)),
		PositionMatrix(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)));
}

void MyApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, RenderLayer name)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->GetResource();
	auto matCB = mCurrFrameResource->MaterialCB->GetResource();

	m_ItemManager->DrawRenderItems(objCBByteSize, objectCB->GetGPUVirtualAddress(),
		matCBByteSize, matCB->GetGPUVirtualAddress(),
		cmdList, name);
}

//大小x旋转 无大小改变为1，无旋转改变为0
FXMMATRIX MyApp::PositionMatrix(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 translate)
{
	return  XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(translate.x, translate.y, translate.z);
}

float MyApp::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 MyApp::GetHillsNormal(float x, float z)const
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