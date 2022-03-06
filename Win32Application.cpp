#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pSample->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0,0,static_cast<LONG>(pSample->GetWidth()),static_cast<LONG>(pSample->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd=CreateWindow(
		windowClass.lpszClassName,
		pSample->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right-windowRect.left,
		windowRect.bottom - windowRect.top, 
		nullptr,        // We have no parent window.
		nullptr,        // We aren't using menus.
		hInstance,
		pSample);

	pSample->OnInit();
	ShowWindow(m_hwnd, nCmdShow);

	MSG msg = {};
	while (msg.message != WM_QUIT) 
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pSample->OnDestroy();
	return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam) 
{
	DXSample* pSample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	switch (message) 
	{
	case WM_CREATE: 
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
		break;

	case WM_KEYDOWN:
	{
		if (pSample) 
		{
			pSample->OnKeyDown(static_cast<UINT8>(wParam));
		}
	}
		break;

	case WM_KEYUP:
		if (pSample)
		{
			pSample->OnKeyUp(static_cast<UINT8>(wParam));
		}
		break;

	case WM_PAINT:
		if (pSample)
		{
			pSample->OnUpdate();
			pSample->OnRender();
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}