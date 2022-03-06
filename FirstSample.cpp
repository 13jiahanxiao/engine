#include"Metalib.h"
#include"FirstSample.h"
#include"uploadBuffer.h"
#include"ReflactableStruct.h"
#include"Common/d3dx12.h"
FirstSample::FirstSample(uint width, uint height, std::wstring name)
{

}

FirstSample::~FirstSample()
{

}

void FirstSample::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void FirstSample::LoadPipeline()
{
	device = std::make_unique<Device>();
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	//½»»»Á´
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;

	ThrowIfFailed(device->GetFactory()->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain));

	ThrowIfFailed(device->GetFactory()->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//ÃèÊö·û
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	m_rtvDescriptorSize = device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (uint32_t n = 0; n < FrameCount; n++) {
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		device->GetDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	ThrowIfFailed(device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void FirstSample::LoadAssets()
{
	ThrowIfFailed(device->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
	std::unique_ptr<UploadBuffer> uploadBuffer;
	{
		// Create a vertex sample
		static Vertex vertexSample;
		std::vector<vbyte> vertexData(vertexSample.structSize * 3);
		vbyte* vertexDataPtr = vertexData.data();
		// Vertex 0
		vertexSample.position.Get(vertexDataPtr) = { 0.0f, 0.25f * m_aspectRatio, 0.0f };
		vertexSample.color.Get(vertexDataPtr) = { 1.0f, 0.0f, 0.0f, 1.0f };
		vertexDataPtr += vertexSample.structSize;
		// Vertex 1
		vertexSample.position.Get(vertexDataPtr) = { 0.25f, -0.25f * m_aspectRatio, 0.0f };
		vertexSample.color.Get(vertexDataPtr) = { 0.0f, 1.0f, 0.0f, 1.0f };
		vertexDataPtr += vertexSample.structSize;
		// Vertex 2
		vertexSample.position.Get(vertexDataPtr) = { -0.25f, -0.25f * m_aspectRatio, 0.0f };
		vertexSample.color.Get(vertexDataPtr) = { 0.0f, 0.0f, 1.0f, 1.0f };
		// Create uploadBuffer
		uploadBuffer = std::make_unique<UploadBuffer>(
			device.get(),
			vertexData.size());
		uploadBuffer->CopyData(0, vertexData);
		// Generate mesh
		std::vector<utility::TypeStruct const*> structs;
		structs.emplace_back(&vertexSample);
		triangleMesh = std::make_unique<Mesh>(
			device.get(),
			structs,
			3);
		// Copy uploadBuffer to mesh
		m_commandList->CopyBufferRegion(
			triangleMesh->VertexBuffers()[0].GetResource(),
			0,
			uploadBuffer->GetResource(),
			0,
			vertexData.size());
	}

	ThrowIfFailed(m_commandList->Close());
	// Execute CommandList
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	// Create the pipeline state, which includes compiling and loading shaders.
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	uint32_t compileFlags = 0;
#endif
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shader/shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shader/shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

	// Define the vertex input layout.

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	auto meshLayout = triangleMesh->Layout();
	psoDesc.InputLayout = { meshLayout.data(), uint(meshLayout.size()) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	ThrowIfFailed(device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValue = 1;

	// Create an event handle to use for frame synchronization.
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
	WaitForPreviousFrame();
}

void FirstSample::OnUpdate()
{
}

void FirstSample::OnRender() {
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void FirstSample::OnDestroy()
{
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void FirstSample::PopulateCommandList() {

	ThrowIfFailed(m_commandAllocator->Reset());

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_commandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
	triangleMesh->GetVertexBufferView(vertexBufferView);
	m_commandList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
	m_commandList->DrawInstanced(3, 1, 0, 0);

	m_commandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));

	ThrowIfFailed(m_commandList->Close());
}

void FirstSample::WaitForPreviousFrame()
{
	const uint64_t fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}